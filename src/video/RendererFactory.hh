// $Id$

#ifndef __RENDERERFACTORY_HH__
#define __RENDERERFACTORY_HH__

#include <string>
#include <map>
#include <SDL/SDL.h>

#include "Settings.hh"
#include "UserEvents.hh"
#include "GLUtil.hh" // for __OPENGL_AVAILABLE__

class Renderer;
class EmuTime;
class VDP;


/** Interface for renderer factories.
  * Every Renderer type has its own RendererFactory.
  * A RendererFactory can be queried about the availability of the
  * associated Renderer and can instantiate that Renderer.
  */
class RendererFactory
{
public:

	/** Enumeration of Renderers known to openMSX.
	  * This is the full list, the list of available renderers may be smaller.
	  */
	enum RendererID { SDLHI, SDLLO, SDLGL, XLIB };

	typedef EnumSetting<RendererID> RendererSetting;

	/** Create the Renderer selected by the current renderer setting.
	  * Use this method for initial renderer creation in the main thread.
	  * @param vdp The VDP whose display will be rendered.
	  */
	static Renderer *createRenderer(VDP *vdp);

	/** Create the Renderer selected by the current renderer setting.
	  * Use this method for changing the renderer in the emulation thread.
	  * @param vdp The VDP whose display will be rendered.
	  */
	static Renderer *switchRenderer(VDP *vdp);

	/** Create the renderer setting.
	  * The map of this setting contains only the available renderers.
	  */
	static RendererSetting *createRendererSetting();

	/** Gets the name of the associated renderer.
	  */
	//virtual const std::string getName() = 0;

	/** Is the associated Renderer available?
	  * Availability may depend on the presence of libraries, the graphics
	  * hardware or the graphics system that is currently running.
	  * This method should return the same value every time it is called.
	  */
	virtual bool isAvailable() = 0;

	/** Instantiate the associated Renderer.
	  * @param vdp VDP whose state will be rendered.
	  * @return A newly created Renderer, or NULL if creation failed.
	  *   TODO: Throwing an exception would be cleaner.
	  */
	virtual Renderer *create(VDP *vdp) = 0;

private:
	/** Get the factory selected by the current renderer setting.
	  * @return The RendererFactory that can create the renderer.
	  */
	static RendererFactory *getCurrent();
};

/** Coordinator for renderer switch operation.
  * This is a complex operation, because renderer creation requires resources,
  * such as SDL screens, which can only be created in the main thread.
  */
class RendererSwitcher
{
private:
	/** VDP whose state will be rendered.
	  */
	VDP *vdp;
	
	/** This field is used to pass a pointer to the newly created renderer
	  * from the main thread to the emulation thread.
	  */
	Renderer * volatile renderer;
	
	/** Semaphore used to suspend emulation thread until main thread has
	  * created the renderer.
	  */
	SDL_sem *semaphore;

public:
	/** Create a new renderer switch operation.
	  * Call performSwitch() to execute the operation.
	  * @param vdp VDP whose state will be rendered.
	  */
	RendererSwitcher(VDP *vdp);
	
	/** Called by the emulation thread to initiate the renderer switch.
	  * Sends an event to the main thread and waits until a response is
	  * received.
	  * @return The newly created renderer.
	  */
	Renderer *performSwitch();
	
	/** Called by the main thread to actually create the new renderer.
	  * Wakes up the emulation thread.
	  */
	void handleEvent();

};

/** Coordinator for full screen toggle operation.
  * The toggle can only be done in the main thread.
  * TODO: Integrate with RendererSwitcher into a generic
  *       "run in another thread" mechanism.
  */
class FullScreenToggler
{
private:
	/** SDL screen whose full screen state will be toggled.
	  */
	SDL_Surface *screen;
	
	/** Semaphore used to suspend emulation thread until main thread has
	  * created the renderer.
	  */
	SDL_sem *semaphore;

public:
	/** Create a new renderer switch operation.
	  * Call performToggle() to execute the operation.
	  * @param screen SDL screen whose full screen state will be toggled.
	  */
	FullScreenToggler(SDL_Surface *screen);
	
	/** Called by the emulation thread to initiate the toggle.
	  * Sends an event to the main thread and waits until a response is
	  * received.
	  */
	void performToggle();
	
	/** Called by the main thread to actually create the new renderer.
	  * Wakes up the emulation thread.
	  */
	void handleEvent();

};

/** RendererFactory for SDLHiRenderer.
  */
class SDLHiRendererFactory: public RendererFactory
{
public:

	/** TODO: Convert to singleton?
	  */
	SDLHiRendererFactory() { }

	const std::string getName() {
		return "SDLHi";
	}

	bool isAvailable();

	Renderer *create(VDP *vdp);

};

/** RendererFactory for SDLLoRenderer.
  */
class SDLLoRendererFactory: public RendererFactory
{
public:

	/** TODO: Convert to singleton?
	  */
	SDLLoRendererFactory() { }

	const std::string getName() {
		return "SDLLo";
	}

	bool isAvailable();

	Renderer *create(VDP *vdp);

};

#ifdef __OPENGL_AVAILABLE__

/** RendererFactory for SDLGLRenderer.
  */
class SDLGLRendererFactory: public RendererFactory
{
public:

	/** TODO: Convert to singleton?
	  */
	SDLGLRendererFactory() { }

	const std::string getName() {
		return "SDLGL";
	}

	bool isAvailable();

	Renderer *create(VDP *vdp);

};

#endif // __OPENGL_AVAILABLE__

/** RendererFactory for XRenderer.
  */
class XRendererFactory: public RendererFactory
{
public:

	/** TODO: Convert to singleton?
	  */
	XRendererFactory() { }

	const std::string getName() {
		return "Xlib";
	}

	bool isAvailable();

	Renderer *create(VDP *vdp);

};

#endif //__RENDERERFACTORY_HH__

