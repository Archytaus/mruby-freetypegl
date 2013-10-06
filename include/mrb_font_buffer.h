#ifndef MRUBY_FONT_BUFFER_H
#define MRUBY_FONT_BUFFER_H

#include <mruby.h>
#include <mruby/class.h>
#include <mruby/data.h>

#include "mrb_freetype_gl.h"
#include "vertex-buffer.h"

struct mrb_font_buffer
{
	texture_atlas_t *atlas;
	texture_font_t *font;
  	vertex_buffer_t *buffer;
  	vec2 min;
  	vec2 max;
};

void mrb_font_buffer_free(mrb_state *mrb, void *p);

void init_mrb_font_buffer(mrb_state* mrb, struct RClass* mrb_sdl_class);
mrb_value mrb_font_buffer_wrap(mrb_state *mrb, texture_atlas_t *atlas, texture_font_t *font, vertex_buffer_t *buffer);
struct mrb_font_buffer* mrb_font_buffer_get_ptr(mrb_state* mrb, mrb_value value);

#endif