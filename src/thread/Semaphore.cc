// $Id$

#include "Semaphore.hh"


Semaphore::Semaphore(unsigned value)
{
	semaphore = SDL_CreateSemaphore(value);
}

Semaphore::~Semaphore()
{
	SDL_DestroySemaphore(semaphore);
}

void Semaphore::up()
{
	SDL_SemPost(semaphore);
}

void Semaphore::down()
{
	SDL_SemWait(semaphore);
}
