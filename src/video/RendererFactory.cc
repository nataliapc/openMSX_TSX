// $Id$

#include "RendererFactory.hh"
#include "openmsx.hh"
#include "RenderSettings.hh"
#include "SDLRenderer.hh"
#include "SDLGLRenderer.hh"
#include "XRenderer.hh"
#include <SDL/SDL.h>


RendererFactory *RendererFactory::getCurrent()
{
	switch (RenderSettings::instance()->getRenderer()->getValue()) {
	case SDLHI:
		return new SDLHiRendererFactory();
	case SDLLO:
		return new SDLLoRendererFactory();
#ifdef __OPENGL_AVAILABLE__
	case SDLGL:
		return new SDLGLRendererFactory();
#endif
	case XLIB:
		return new XRendererFactory();
	default:
		throw MSXException("Unknown renderer");
	}
}

Renderer *RendererFactory::createRenderer(VDP *vdp)
{
	return getCurrent()->create(vdp);
}

Renderer *RendererFactory::switchRenderer(VDP *vdp)
{
	RendererSwitcher switcher(vdp);
	return switcher.performSwitch();
}

RendererFactory::RendererSetting *RendererFactory::createRendererSetting()
{
	std::map<std::string, RendererID> rendererMap;
	rendererMap["SDLHi"] = SDLHI;
	rendererMap["SDLLo"] = SDLLO;
#ifdef __OPENGL_AVAILABLE__
	rendererMap["SDLGL"] = SDLGL;
#endif
	// XRenderer is not ready for users.
	// rendererMap["Xlib" ] = XLIB;
	return new RendererSetting(
		"renderer", "rendering back-end used to display the MSX screen",
		SDLHI, rendererMap
		);
}

// RendererSwitcher ========================================================

RendererSwitcher::RendererSwitcher(VDP *vdp) {
	this->vdp = vdp;
	renderer = NULL;
	semaphore = SDL_CreateSemaphore(0);
}

Renderer *RendererSwitcher::performSwitch() {
	// Push switch event into event queue.
	UserEvents::push(UserEvents::RENDERER_SWITCH, this);
	// Wait until main thread has performed its work.
	SDL_SemWait(semaphore);
	return renderer;
}

void RendererSwitcher::handleEvent() {
	// Actually create renderer.
	renderer = RendererFactory::createRenderer(vdp);
	// Notify emulation thread.
	SDL_SemPost(semaphore);
}

// FullScreenToggler =======================================================

FullScreenToggler::FullScreenToggler(SDL_Surface *screen) {
	this->screen = screen;
	semaphore = SDL_CreateSemaphore(0);
}

void FullScreenToggler::performToggle() {
	// Push switch event into event queue.
	UserEvents::push(UserEvents::FULL_SCREEN_TOGGLE, this);
	// Wait until main thread has performed its work.
	SDL_SemWait(semaphore);
}

void FullScreenToggler::handleEvent() {
	// Actually toggle.
	SDL_WM_ToggleFullScreen(screen);
	// Notify emulation thread.
	SDL_SemPost(semaphore);
}

// SDLHi ===================================================================

bool SDLHiRendererFactory::isAvailable()
{
	return true; // TODO: Actually query.
}

Renderer *SDLHiRendererFactory::create(VDP *vdp)
{
	const int WIDTH = 640;
	const int HEIGHT = 480;

	bool fullScreen = RenderSettings::instance()->getFullScreen()->getValue();
	int flags = SDL_HWSURFACE | (fullScreen ? SDL_FULLSCREEN : 0);

	// Try default bpp.
	SDL_Surface *screen = SDL_SetVideoMode(WIDTH, HEIGHT, 0, flags);

	// If no screen or unsupported screen,
	// try supported bpp in order of preference.
	int bytepp = (screen ? screen->format->BytesPerPixel : 0);
	if (bytepp != 1 && bytepp != 2 && bytepp != 4) {
		if (!screen) screen = SDL_SetVideoMode(WIDTH, HEIGHT, 15, flags);
		if (!screen) screen = SDL_SetVideoMode(WIDTH, HEIGHT, 16, flags);
		if (!screen) screen = SDL_SetVideoMode(WIDTH, HEIGHT, 32, flags);
		if (!screen) screen = SDL_SetVideoMode(WIDTH, HEIGHT, 8, flags);
	}

	if (!screen) {
		printf("FAILED to open any screen!");
		// TODO: Throw exception.
		return NULL;
	}
	PRT_DEBUG("Display is " << (int)(screen->format->BitsPerPixel) << " bpp.");

	switch (screen->format->BytesPerPixel) {
	case 1:
		return new SDLRenderer<Uint8, Renderer::ZOOM_512>(SDLHI, vdp, screen);
	case 2:
		return new SDLRenderer<Uint16, Renderer::ZOOM_512>(SDLHI, vdp, screen);
	case 4:
		return new SDLRenderer<Uint32, Renderer::ZOOM_512>(SDLHI, vdp, screen);
	default:
		printf("FAILED to open supported screen!");
		return NULL;
	}
}

// SDLLo ===================================================================

bool SDLLoRendererFactory::isAvailable()
{
	return true; // TODO: Actually query.
}

Renderer *SDLLoRendererFactory::create(VDP *vdp)
{
	const int WIDTH = 320;
	const int HEIGHT = 240;

	bool fullScreen = RenderSettings::instance()->getFullScreen()->getValue();
	int flags = SDL_HWSURFACE | (fullScreen ? SDL_FULLSCREEN : 0);

	// Try default bpp.
	SDL_Surface *screen = SDL_SetVideoMode(WIDTH, HEIGHT, 0, flags);

	// If no screen or unsupported screen,
	// try supported bpp in order of preference.
	int bytepp = (screen ? screen->format->BytesPerPixel : 0);
	if (bytepp != 1 && bytepp != 2 && bytepp != 4) {
		if (!screen) screen = SDL_SetVideoMode(WIDTH, HEIGHT, 15, flags);
		if (!screen) screen = SDL_SetVideoMode(WIDTH, HEIGHT, 16, flags);
		if (!screen) screen = SDL_SetVideoMode(WIDTH, HEIGHT, 32, flags);
		if (!screen) screen = SDL_SetVideoMode(WIDTH, HEIGHT, 8, flags);
	}

	if (!screen) {
		printf("FAILED to open any screen!");
		// TODO: Throw exception.
		return NULL;
	}
	PRT_DEBUG("Display is " << (int)(screen->format->BitsPerPixel) << " bpp.");

	switch (screen->format->BytesPerPixel) {
	case 1:
		return new SDLRenderer<Uint8, Renderer::ZOOM_256>(SDLLO, vdp, screen);
	case 2:
		return new SDLRenderer<Uint16, Renderer::ZOOM_256>(SDLLO, vdp, screen);
	case 4:
		return new SDLRenderer<Uint32, Renderer::ZOOM_256>(SDLLO, vdp, screen);
	default:
		printf("FAILED to open supported screen!");
		// TODO: Throw exception.
		return NULL;
	}

}

// SDLGL ===================================================================

#ifdef __OPENGL_AVAILABLE__

bool SDLGLRendererFactory::isAvailable()
{
	return true; // TODO: Actually query.
}

Renderer *SDLGLRendererFactory::create(VDP *vdp)
{
	const int WIDTH = 640;
	const int HEIGHT = 480;

	bool fullScreen = RenderSettings::instance()->getFullScreen()->getValue();
	int flags = SDL_OPENGL | SDL_HWSURFACE |
	            (fullScreen ? SDL_FULLSCREEN : 0);

	// Enables OpenGL double buffering.
	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, true);

	// Try default bpp.
	SDL_Surface *screen = SDL_SetVideoMode(WIDTH, HEIGHT, 0, flags);

	// If no screen or unsupported screen,
	// try supported bpp in order of preference.
	int bytepp = (screen ? screen->format->BytesPerPixel : 0);
	if (bytepp != 1 && bytepp != 2 && bytepp != 4) {
		if (!screen) screen = SDL_SetVideoMode(WIDTH, HEIGHT, 15, flags);
		if (!screen) screen = SDL_SetVideoMode(WIDTH, HEIGHT, 16, flags);
		if (!screen) screen = SDL_SetVideoMode(WIDTH, HEIGHT, 32, flags);
		if (!screen) screen = SDL_SetVideoMode(WIDTH, HEIGHT, 8, flags);
	}

	if (!screen) {
		printf("FAILED to open any screen!");
		// TODO: Throw exception.
		return NULL;
	}
	PRT_DEBUG("Display is " << (int)(screen->format->BitsPerPixel) << " bpp.");

	return new SDLGLRenderer(SDLGL, vdp, screen);
}

#endif // __OPENGL_AVAILABLE__

// Xlib ====================================================================

bool XRendererFactory::isAvailable()
{
	return true; // TODO: Actually query.
}

Renderer *XRendererFactory::create(VDP *vdp)
{
	return new XRenderer(XLIB, vdp);
}

