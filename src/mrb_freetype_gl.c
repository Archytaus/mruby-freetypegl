#include "mrb_freetype_gl.h"

#include <mruby/array.h>
#include "mrb_font_buffer.h"

static struct RClass* mrb_freetype_gl_class = NULL;

void
mrb_mruby_freetype_gl_gem_init(mrb_state* mrb) {
  mrb_freetype_gl_class = mrb_define_module(mrb, "FreetypeGL");
  init_mrb_font_buffer(mrb, mrb_freetype_gl_class);
}