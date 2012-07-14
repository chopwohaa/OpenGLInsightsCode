/*
Copyright (C) 2011 by Ladislav Hrabcak

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
*/

#include "renderer.h"
#include "scene.h"

#include "base/base.h"
#include "base/canvas.h"
#include "base/frame_context.h"
#include "base/app.h"

#include <iostream>

#include <glm/glm.hpp>

using namespace glm;

//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

renderer::renderer(base::app *a, const bool create_shared_context)
	: thread()
	, _event()
	, _queue()
	, _mx_queue()
	, _shutdown(false)
	, _app(a)
	, _create_shared_context(create_shared_context)
{}

//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

renderer::~renderer() {}

//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

void renderer::start(const base::source_location &loc)
{
	thread::start(loc);

	if(!_event.wait(10000))
		throw base::exception(loc.to_str())
			<< "Renderer initialization failed!";
}

//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

void renderer::run()
{
	base::init_opengl_win(_create_shared_context);
	base::init_opengl_dbg_win();

	// create frame_context pool
	{
		base::mutex_guard g(_mx_queue);

		base::app::get()->create_frame_context_pool(!_create_shared_context);
	}

	base::canvas::load_and_init_shaders(SRC_LOCATION);
	scene::load_and_init_shaders(SRC_LOCATION);

	_app->renderer_side_start();

	_event.signal();

	for(;;) {
		base::frame_context *ctx = 0;

		if(!_waiting.empty() && _waiting.front()->check_fence()) {
			if(!_create_shared_context) {
				_waiting.front()->map_scene();
				_waiting.front()->map_canvas();
			}

			base::mutex_guard g(_mx_queue);
			base::app::get()->push_frame_context_to_pool(_waiting.front());
			
			_waiting.pop_front();
		}		

		{
			base::mutex_guard g(_mx_queue);
			if(!_queue.empty()) {
				ctx = _queue.back();
				_queue.pop_back();
			}
		}

		if(ctx==0) {
			if(_shutdown) break;
			continue;
		}

		// start transfer of data to device memory
		if(!_create_shared_context) {
			ctx->flush_scene_data();
			ctx->flush_canvas_data();
		}

		draw_frame(ctx);

		ctx->put_fence();
		_waiting.push_back(ctx);

		if(_shutdown)
			break;
	}

	//TODO purge frame context pool

	std::cout << "Renderer thread ended...\n";
}

//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

void renderer::stop(const base::source_location &loc)
{
	assert(_shutdown==false);

	std::cout << "Renderer is going down...\n";
	
	_shutdown = true;
	_event.signal();

	if(!wait_for_end(2000)) {
		std::cout << "Terminating renderer thread!\n";
		terminate();
	}

	thread::stop(loc);
}

//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

void renderer::draw_frame(base::frame_context *ctx)
{
	glEnable(GL_CULL_FACE);
	glEnable(GL_DEPTH_TEST);
    glClearColor(0.3f, 0.2f, 0.4f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	scene::render_blocks(ctx);
	base::canvas::render(ctx);

	if(_create_shared_context) {
		//_app->capture_screen();
	}

	base::swap_buffers();
}

//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

base::frame_context* renderer::pop_frame_context_from_pool() {
	base::mutex_guard g(_mx_queue);
	return base::app::get()->pop_frame_context_from_pool();
}

//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
