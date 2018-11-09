#include <iostream>

#include "DeepMimicCore.h"

#include "util/FileUtil.h"

#include "render/DrawUtil.h"
#include "render/TextureDesc.h"
#include "OpenGLWindow/SimpleOpenGL3App.h"
#include "OpenGLWindow/b3Clock.h"

static SimpleOpenGL3App* s_app=0;
static b3Clock s_clock;

static b3WheelCallback sOldWheelCB = 0;
static b3ResizeCallback sOldResizeCB = 0;
static b3MouseMoveCallback sOldMouseMoveCB = 0;
static b3MouseButtonCallback sOldMouseButtonCB = 0;
static b3KeyboardCallback sOldKeyboardCB = 0;
//static b3RenderCallback sOldRenderCB = 0;

static float gWidth = 1024;
static float gHeight = 768;

void MyWheelCallback2(float deltax, float deltay)
{
        if (sOldWheelCB)
                sOldWheelCB(deltax, deltay);
}

// Dimensions of the window we are drawing into.
int gWinWidth = 800;
int gWinHeight = static_cast<int>(gWinWidth * 9.0 / 16.0);
//int gWinWidth = 1055;
//int gWinHeight = 450;
bool gReshaping = false;

// intermediate frame buffers
std::unique_ptr<cTextureDesc> gDefaultFrameBuffer;

// anim
const double gFPS = 60.0;
const double gAnimStep = 1.0 / gFPS;
const int gDisplayAnimTime = static_cast<int>(1000 * gAnimStep);
bool gAnimating = true;

int gSampleCount = 0;

double gPlaybackSpeed = 1;
const double gPlaybackDelta = 0.05;

// FPS counter
int gPrevTime = 0;
double gUpdatesPerSec = 0;

std::vector<std::string> gArgs;
std::unique_ptr<cDeepMimicCore> gCore;

void SetupDeepMimicCore()
{
	bool enable_draw = true;
	gCore = std::unique_ptr<cDeepMimicCore>(new cDeepMimicCore(enable_draw));
	gCore->ParseArgs(gArgs);
	gCore->Init();

	int num_agents = gCore->GetNumAgents();
	for (int id = 0; id < num_agents; ++id)
	{
		int action_space = gCore->GetActionSpace(id);
		int state_size = gCore->GetStateSize(id);
		int goal_size = gCore->GetGoalSize(id);
		int action_size = gCore->GetActionSize(id);
		int num_actions = gCore->GetNumActions(id);

		auto s_offset = gCore->BuildStateOffset(id);
		auto s_scale = gCore->BuildStateScale(id);
		auto g_offset = gCore->BuildGoalOffset(id);
		auto g_scale = gCore->BuildGoalScale(id);
		auto a_offset = gCore->BuildActionOffset(id);
		auto a_scale = gCore->BuildActionScale(id);

		auto action_min = gCore->BuildActionBoundMin(id);
		auto action_max = gCore->BuildActionBoundMax(id);

		auto state_norm_groups = gCore->BuildStateNormGroups(id);
		auto goal_norm_groups = gCore->BuildGoalNormGroups(id);

		double reward_min = gCore->GetRewardMin(id);
		double reward_max = gCore->GetRewardMax(id);
		double reward_fail = gCore->GetRewardFail(id);
		double reward_succ = gCore->GetRewardSucc(id);

		int xx = 0;
		++xx;
	}
}

void FormatArgs(int argc, char** argv, std::vector<std::string>& out_args)
{
	out_args.resize(argc);
	for (int i = 0; i < argc; ++i)
	{
		out_args[i] = std::string(argv[i]);
	}
}

void UpdateFrameBuffer()
{
	if (!gReshaping)
	{
		if (gWinWidth != gCore->GetWinWidth() || gWinHeight != gCore->GetWinHeight())
		{
			gCore->Reshape(gWinWidth, gWinHeight);
		}
	}
}

void Update(double time_elapsed)
{
	int num_substeps = gCore->GetNumUpdateSubsteps();
	double timestep = time_elapsed / num_substeps;
	num_substeps = (time_elapsed == 0) ? 1 : num_substeps;

	for (int i = 0; i < num_substeps; ++i)
	{
		for (int id = 0; id < gCore->GetNumAgents(); ++id)
		{
			if (gCore->NeedNewAction(id))
			{
				auto s = gCore->RecordState(id);
				auto g = gCore->RecordGoal(id);
				double r = gCore->CalcReward(id);
				++gSampleCount;

				std::vector<double> action = std::vector<double>(gCore->GetActionSize(id), 0);
				gCore->SetAction(id, action);
			}
		}

		gCore->Update(timestep);

		if (gCore->IsRLScene())
		{
			bool end_episode = gCore->IsEpisodeEnd();
			bool valid_episode = gCore->CheckValidEpisode();
			if (end_episode || !valid_episode)
			{
				for (int id = 0; id < gCore->GetNumAgents(); ++id)
				{
					int terminated = gCore->CheckTerminate(id);
					if (terminated)
					{
						printf("Agent %i terminated\n", id);
					}
				}
				gCore->SetSampleCount(gSampleCount);
				gCore->Reset();
			}
		}
	}
}

void Draw(void)
{
	UpdateFrameBuffer();
	gCore->Draw();
	
	gReshaping = false;
}

void Reshape(int w, int h)
{
	gReshaping = true;

	gWinWidth = w;
	gWinHeight = h;
	
	gDefaultFrameBuffer->Reshape(w, h);
	glViewport(0, 0, gWinWidth, gWinHeight);
}

void MyResizeCallback2(float width, float height)
{       
        gWidth = width;
        gHeight = height;

        if (sOldResizeCB)
                sOldResizeCB(width, height);
	Reshape((int)width, (int)height);
}

void StepAnim(double time_step)
{
	Update(time_step);
	gAnimating = false;
}

void Reload()
{
	SetupDeepMimicCore();
}

void Reset()
{
	gCore->Reset();
}

int GetNumTimeSteps()
{
	int num_steps = static_cast<int>(gPlaybackSpeed);
	if (num_steps == 0)
	{
		num_steps = 1;
	}
	num_steps = std::abs(num_steps);
	return num_steps;
}

int CalcDisplayAnimTime(int num_timesteps)
{
	int anim_time = static_cast<int>(gDisplayAnimTime * num_timesteps / gPlaybackSpeed);
	anim_time = std::abs(anim_time);
	return anim_time;
}

void Shutdown()
{
	gCore->Shutdown();
	exit(0);
}

int GetCurrTime()
{
	return s_clock.getTimeMilliseconds();
}

void InitTime()
{
	gPrevTime = GetCurrTime();
	gUpdatesPerSec = 0;
}

void Animate()
{
	const double counter_decay = 0;

	if (gAnimating)
	{
		int num_steps = GetNumTimeSteps();
		int curr_time = GetCurrTime();
		int time_elapsed = curr_time - gPrevTime;
		gPrevTime = curr_time;

		double timestep = (gPlaybackSpeed < 0) ? -gAnimStep : gAnimStep;
		for (int i = 0; i < num_steps; ++i)
		{
			Update(timestep);
		}
		
		// FPS counting
		double update_count = num_steps / (0.001 * time_elapsed);
		if (std::isfinite(update_count))
		{
			gUpdatesPerSec = counter_decay * gUpdatesPerSec + (1 - counter_decay) * update_count;
			gCore->SetUpdatesPerSec(gUpdatesPerSec);
		}

		int timer_step = CalcDisplayAnimTime(num_steps);
		int update_dur = GetCurrTime() - curr_time;
		timer_step -= update_dur;
		timer_step = std::max(timer_step, 0);
		
	}

	if (gCore->IsDone())
	{
		Shutdown();
	}
}

void ToggleAnimate()
{
	gAnimating = !gAnimating;
}

void ChangePlaybackSpeed(double delta)
{
	double prev_playback = gPlaybackSpeed;
	gPlaybackSpeed += delta;
	gCore->SetPlaybackSpeed(gPlaybackSpeed);

}

void Keyboard(unsigned char key, int x, int y) 
{
	gCore->Keyboard(key, x, y);

	switch (key) {
	case 27: // escape
		Shutdown();
		break;
	case ' ':
		ToggleAnimate();
		break;
	case '>':
		StepAnim(gAnimStep);
		break;
	case '<':
		StepAnim(-gAnimStep);
		break;
	case ',':
		ChangePlaybackSpeed(-gPlaybackDelta);
		break;
	case '.':
		ChangePlaybackSpeed(gPlaybackDelta);
		break;
	case '/':
		ChangePlaybackSpeed(-gPlaybackSpeed + 1);
		break;
	case 'l':
		Reload();
		break;
	case 'r':
		Reset();
		break;
	default:
		break;
	}

}

void MouseClick(int button, int state, int x, int y)
{
	gCore->MouseClick(button, state, x, y);
}

void MouseMove(int x, int y)
{
	gCore->MouseMove(x, y);
}

void InitFrameBuffers(void)
{
	gDefaultFrameBuffer = std::unique_ptr<cTextureDesc>(new cTextureDesc(0, 0, 0, gWinWidth, gWinHeight, 1, GL_RGBA, GL_RGBA));
}

void InitDraw(int argc, char** argv)
{
	printf("InitDraw!\n");
	s_app = new SimpleOpenGL3App("DeepMimic", gWinWidth, gWinHeight);
	printf("Finished InitDraw\n");
}

void SetupDraw()
{

#if 0
        s_app->m_renderer->getActiveCamera()->setCameraDistance(13);
        s_app->m_renderer->getActiveCamera()->setCameraPitch(0);
        s_app->m_renderer->getActiveCamera()->setCameraTargetPosition(0, 0, 0);
        sOldKeyboardCB = s_app->m_window->getKeyboardCallback();
        s_app->m_window->setKeyboardCallback(MyKeyboardCallback2);
        sOldMouseMoveCB = app->m_window->getMouseMoveCallback();
        s_app->m_window->setMouseMoveCallback(MyMouseMoveCallback2);
        sOldMouseButtonCB = s_app->m_window->getMouseButtonCallback();
        s_app->m_window->setMouseButtonCallback(MyMouseButtonCallback2);
        sOldWheelCB = s_app->m_window->getWheelCallback();
        s_app->m_window->setWheelCallback(MyWheelCallback2);
#endif
        sOldResizeCB = s_app->m_window->getResizeCallback();
        s_app->m_window->setResizeCallback(MyResizeCallback2);

//	glutDisplayFunc(Draw);
//	glutReshapeFunc(Reshape);
//	glutKeyboardFunc(Keyboard);
//	glutMouseFunc(MouseClick);
//	glutMotionFunc(MouseMove);
//	glutTimerFunc(gDisplayAnimTime, Animate, 0);

	InitFrameBuffers();
//	Reshape(gWinWidth, gWinHeight);
	gCore->Reshape(gWinWidth, gWinHeight);
}

void DrawMainLoop()
{
	InitTime();

	do
	{
		Animate();
		Draw();
		s_app->swapBuffer();
	} while (!s_app->m_window->requestedExit());

}

int main(int argc, char** argv)
{
	FormatArgs(argc, argv, gArgs);

	InitDraw(argc, argv);
	SetupDeepMimicCore();
	SetupDraw();

	DrawMainLoop();

	delete s_app;

	return EXIT_SUCCESS;
}

