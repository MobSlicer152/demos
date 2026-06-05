#include "pch.h"

bool g_running;
bool g_paused;

uint64_t g_lastTime;
uint64_t g_nowTime;
float g_delta;
float g_elapsed;
int g_targetFps = DEFAULT_TARGET_FPS;

uint32_t g_width;
uint32_t g_height;
float g_aspect;

static byte s_fbMem[FRAMEBUFFER_WIDTH * FRAMEBUFFER_HEIGHT];
byte* g_framebuffer = s_fbMem;
float g_zBuffer[FRAMEBUFFER_WIDTH * FRAMEBUFFER_HEIGHT];
uint32_t g_fbStride;
bool g_autoClear = true;
Vec4 g_clearColor = Vec4::BLACK;

static Display* s_display;
static XVisualInfo s_visual;
static Window s_window;
static uint32_t s_fbImageMem[FRAMEBUFFER_WIDTH * FRAMEBUFFER_HEIGHT];
static XImage* s_fbImage;
PaletteColor g_palette[PALETTE_SIZE];

static FILE* s_urandom = fopen("/dev/urandom", "rb");
float UniformRandom(float min, float max)
{
	int result = 0;
	fread(&result, sizeof(float), 1, s_urandom);
	return std::clamp(((float)result / UINT32_MAX) * (max - min) + min, min, max);
}

uint64_t GetTickCount64()
{
	struct timespec ts;
	uint64_t val = 0;
	clock_gettime(CLOCK_REALTIME, &ts);
	val = ts.tv_nsec / 1000000;
	val += ts.tv_sec * 1000;
	return val;
}

static void InitWindow()
{
	auto screen = DefaultScreen(s_display);
	auto root = DefaultRootWindow(s_display);

	auto screenWidth = DisplayWidth(s_display, screen);
	auto screenHeight = DisplayHeight(s_display, screen);

	// make a nice 4:3 that's 3/4 the height of the monitor
	g_height = (screenHeight * 0.75);
	g_width = g_height * 1.33333f;
	g_aspect = (float)g_width / g_height;

	// centre and account for caption
	int x = (screenWidth - g_width) / 2;
	int y = (screenHeight - g_height) / 2;

	// get visual info for 8 bit indexed colors
	auto depth = 24;
	if (!XMatchVisualInfo(s_display, screen, depth, TrueColor, &s_visual))
	{
		ErrorMessage(EXIT_FAILURE, "failed to find matching visual");
	}

	XSetWindowAttributes attrs = {};
	attrs.background_pixel = 0;													   // black
	attrs.colormap = XCreateColormap(s_display, root, s_visual.visual, AllocNone); // colormap for the visual
	attrs.event_mask = StructureNotifyMask;
	auto attrMask = CWBackPixel | CWColormap | CWEventMask;

	// create the window
	s_window = XCreateWindow(
		s_display, root, x, y, g_width, g_height, 0, s_visual.depth, InputOutput, s_visual.visual, attrMask, &attrs);
	if (!s_window)
	{
		ErrorMessage(EXIT_FAILURE, "failed to create window");
	}

	// set title
	XStoreName(s_display, s_window, "Demo");

	// disallow resizing
	XSizeHints sizeHints = {};
	sizeHints.min_width = g_width;
	sizeHints.max_width = g_width;
	sizeHints.min_height = g_height;
	sizeHints.max_height = g_height;
	sizeHints.flags = PMinSize | PMaxSize;
	XSetWMNormalHints(s_display, s_window, &sizeHints);

	XMapWindow(s_display, s_window);
}

DECLSPEC_NORETURN void ErrorMessage(int code, const char* msg, ...)
{
	va_list args;
	va_start(args, msg);
	ErrorMessage(code, msg, args);
}

DECLSPEC_NORETURN void ErrorMessage(int code, const char* msg, va_list args)
{
	vprintf(msg, args);
	puts(""); // newline
	exit(code);
}

static void InitFramebuffer()
{
	// create framebuffer image
	s_fbImage = XCreateImage(
		s_display,
		s_visual.visual,
		s_visual.depth,
		ZPixmap,
		0,
		(char*)s_fbImageMem,
		FRAMEBUFFER_WIDTH,
		FRAMEBUFFER_HEIGHT,
		32,
		0);
	if (!s_fbImage)
	{
		ErrorMessage(EXIT_FAILURE, "failed to create framebuffer image");
	}

	g_fbStride = FRAMEBUFFER_WIDTH;
}

static void SleepToNextFrame()
{
	float spf = 1.0f / g_targetFps;
	float delay = spf - g_delta;
	if (delay > 0.0f)
	{
		usleep(delay * 1000000);
	}
}

static void WindowProc(XEvent& event)
{
	switch (event.type)
	{
	case DestroyNotify: {
		XDestroyWindowEvent& e = *(XDestroyWindowEvent*)&event;
		if (e.window == s_window)
		{
			g_running = false;
		}
		break;
	}
	}
}

static void ConvertFramebuffer()
{
	for (auto y = 0; y < FRAMEBUFFER_HEIGHT; y++)
	{
		for (auto x = 0; x < FRAMEBUFFER_WIDTH; x++)
		{
			// look up the color, map it to an x11 pixel, and write it to the x11 framebuffer
			auto index = g_framebuffer[y * FRAMEBUFFER_WIDTH + x];
			auto& orig = g_palette[index];
			uint32_t out = ((uint32_t)orig.r << 16) | ((uint32_t)orig.g << 8) | ((uint32_t)orig.b << 0);
			s_fbImageMem[y * FRAMEBUFFER_WIDTH + x] = out;
		}
	}
}

static void WindowLoop()
{
	g_lastTime = GetTickCount64();
	g_running = true;
	while (g_running)
	{
		g_nowTime = GetTickCount64();

		XEvent event = {};
		while (XPending(s_display) > 0)
		{
			XNextEvent(s_display, &event);
			WindowProc(event);
		}

		if (g_autoClear)
		{
			DrawRectangle(Vec2(0.0f), Vec2(1.0f), g_clearColor);
			ClearColor(g_clearColor);
			ClearDepth(FLT_MIN);
		}

		DrawDemo();
		ConvertFramebuffer();

		// TODO: xrender
		auto screen = DefaultScreen(s_display);
		auto gc = DefaultGC(s_display, screen);
		XPutImage(s_display, s_window, gc, s_fbImage, 0, 0, 0, 0, FRAMEBUFFER_WIDTH, FRAMEBUFFER_HEIGHT);

		g_delta = g_paused ? 0.0f : (g_nowTime - g_lastTime) / 1000.0f;
		SleepToNextFrame();
		g_elapsed += g_delta;
		g_lastTime = g_nowTime;
	}
}

// calculates similar colors for dithering
extern void InitColorTable();

// generates perlin permutation table
extern void InitNoise();

int main(int argc, char* argv[])
{
	s_display = XOpenDisplay(nullptr);
	if (!s_display)
	{
		ErrorMessage(EXIT_FAILURE, "failed to open display");
	}

	InitWindow();
	InitNoise();
	InitDemoPalette();
	InitColorTable();
	InitFramebuffer();
	InitDemo();

	XRaiseWindow(s_display, s_window);
	XFlush(s_display);

	WindowLoop();
	ShutdownDemo();
	// ShutdownWindow();

	XCloseDisplay(s_display);

	fclose(s_urandom);

	return 0;
}
