#include "mrb_font_buffer.h"

#include <stdio.h>
#include <string.h>

#include <mrbconf.h>
#include <mruby.h>
#include <mruby/class.h>
#include <mruby/data.h>
#include <mruby/value.h>
#include <mruby/string.h>
#include <mruby/variable.h>

#include "mrb_vec2.h"

typedef struct {
    float x, y, z;    // position
    float s, t;       // texture
    float r, g, b, a; // color
} vertex_t;

static struct RClass* mrb_font_buffer_class = NULL;

struct mrb_data_type mrb_font_buffer_get_ptr_type = { "FontBuffer", mrb_font_buffer_free };

void
mrb_font_buffer_free(mrb_state *mrb, void *ptr)
{
  mrb_free(mrb, ptr);
}

mrb_value
mrb_font_buffer_wrap(mrb_state *mrb, texture_atlas_t *atlas, texture_font_t *font, vertex_buffer_t *buffer)
{
  struct mrb_font_buffer* tm = (struct mrb_font_buffer*)mrb_malloc(mrb, sizeof(struct mrb_font_buffer));
  tm->atlas = atlas;
  tm->font = font;
  tm->buffer = buffer;
  return mrb_obj_value(Data_Wrap_Struct(mrb, mrb_font_buffer_class, &mrb_font_buffer_get_ptr_type, tm));
}

struct mrb_font_buffer*
mrb_font_buffer_get_ptr(mrb_state* mrb, mrb_value value)
{
  return (struct mrb_font_buffer*)mrb_data_get_ptr(mrb, value, &mrb_font_buffer_get_ptr_type);
}

mrb_value
mrb_font_buffer_create(mrb_state *mrb, mrb_value self)
{
  mrb_value font;
  mrb_int size;

  mrb_get_args(mrb, "Si", &font, &size);


  GLuint vao = 0;
  
  glGenVertexArrays(1, &vao);
  glBindVertexArray(vao);

  /* Texture atlas to store individual glyphs */
  texture_atlas_t *atlas = texture_atlas_new( 512, 512, 1 );

  /* Build a new texture font from its description and size */
  texture_font_t *texture_font = texture_font_new(atlas, mrb_string_value_ptr(mrb, font), size);

  /* Build a new vertex buffer (position, texture & color) */
  vertex_buffer_t *buffer = vertex_buffer_new( "vertex:3f,tex_coord:2f,color:4f" );

  /* Cache some glyphs to speed things up */
  texture_font_load_glyphs( texture_font, L" !\"#$%&'()*+,-./0123456789:;<=>?"
                             L"@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_"
                             L"`abcdefghijklmnopqrstuvwxyz{|}~");

  mrb_value wrapped_font_buffer = mrb_font_buffer_wrap(mrb, atlas, texture_font, buffer);
  mrb_value vao_id = mrb_fixnum_value(vao);
  mrb_iv_set(mrb, wrapped_font_buffer, mrb_intern(mrb, "@vao_id"), vao_id);

  glBindVertexArray(0);

  return wrapped_font_buffer;
}

void add_text( vertex_buffer_t * buffer, texture_font_t * font,
               char* text, vec4 * color )
{
    vec2 pen = {{0,0}};
    size_t i;
    float r = color->red, g = color->green, b = color->blue, a = color->alpha;
    for( i=0; i<strlen(text); ++i )
    {
        texture_glyph_t *glyph = texture_font_get_glyph( font, text[i] );
        if( glyph != NULL )
        {
            int kerning = 0;
            if( i > 0)
            {
                kerning = texture_glyph_get_kerning( glyph, text[i-1] );
            }
            pen.x += kerning;
            int x0  = (int)( pen.x + glyph->offset_x );
            int y0  = (int)( pen.y - glyph->offset_y );
            int x1  = (int)( x0 + glyph->width );
            int y1  = (int)( y0 + glyph->height );
            float s0 = glyph->s0;
            float t0 = glyph->t0;
            float s1 = glyph->s1;
            float t1 = glyph->t1;
            GLuint indices[6] = {0,1,2, 0,2,3};
            vertex_t vertices[4] = { { x0,y0,0,  s0,t0,  r,g,b,a },
                                     { x0,y1,0,  s0,t1,  r,g,b,a },
                                     { x1,y1,0,  s1,t1,  r,g,b,a },
                                     { x1,y0,0,  s1,t0,  r,g,b,a } };
            vertex_buffer_push_back( buffer, vertices, 4, indices, 6 );
            pen.x += glyph->advance_x;
        }
    }
}

mrb_value
mrb_font_buffer_set_text(mrb_state *mrb, mrb_value self)
{
  struct mrb_font_buffer* font_buffer = mrb_font_buffer_get_ptr(mrb, self);

  mrb_value text;
  mrb_get_args(mrb, "S", &text);

  vec4 black = {{0,0,0,1}};

  vertex_buffer_clear(font_buffer->buffer);
  add_text(font_buffer->buffer, font_buffer->font, mrb_string_value_ptr(mrb, text), &black);

  return self;
}

mrb_value
mrb_font_buffer_bind(mrb_state *mrb, mrb_value self)
{
  struct mrb_font_buffer* font_buffer = mrb_font_buffer_get_ptr(mrb, self);

  mrb_int program_id;
  int arg_count = mrb_get_args(mrb, "i", &program_id);

  glBindTexture( GL_TEXTURE_2D, font_buffer->atlas->id );
  vertex_buffer_render_setup( font_buffer->buffer, GL_TRIANGLES );
  
  mrb_value vao_id = mrb_iv_get(mrb, self, mrb_intern(mrb, "@vao_id"));
  GLuint vao = mrb_fixnum(vao_id);
  glBindVertexArray(vao);

  if (arg_count > 0) {
    
    glUseProgram(program_id);

    // connect the xyz to the "vert" attribute of the vertex shader
    GLint attrib = glGetAttribLocation(program_id, "vertex");  
    glEnableVertexAttribArray(attrib);
    glVertexAttribPointer(attrib, 3, GL_FLOAT, GL_FALSE, 9*sizeof(GLfloat), NULL);

    GLint attribTex = glGetAttribLocation(program_id, "tex_coord");  
    glEnableVertexAttribArray(attribTex);
    glVertexAttribPointer(attribTex, 2, GL_FLOAT, GL_TRUE, 9*sizeof(GLfloat), (const GLvoid*)(3 * sizeof(GLfloat)));

    GLint attribColor = glGetAttribLocation(program_id, "color");  
    glEnableVertexAttribArray(attribColor);
    glVertexAttribPointer(attribColor, 4, GL_FLOAT, GL_TRUE, 9*sizeof(GLfloat), (const GLvoid*)(5 * sizeof(GLfloat)));
  }
  
  return self;
}

mrb_value
mrb_font_buffer_unbind(mrb_state *mrb, mrb_value self)
{
  struct mrb_font_buffer* font_buffer = mrb_font_buffer_get_ptr(mrb, self);
  
  glUseProgram(0);
  glBindVertexArray(0);
  vertex_buffer_render_finish(font_buffer->buffer);
  glBindTexture(GL_TEXTURE_2D, 0);

  return self;
}

mrb_value
mrb_font_buffer_render(mrb_state *mrb, mrb_value self)
{
  struct mrb_font_buffer* font_buffer = mrb_font_buffer_get_ptr(mrb, self);

  size_t vcount = font_buffer->buffer->vertices->size;
  size_t icount = font_buffer->buffer->indices->size;

  if( icount )
  {
      glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, font_buffer->buffer->indices_id );
      glDrawElements( GL_TRIANGLES, icount, GL_UNSIGNED_INT, 0 );
  }
  else
  {
      glDrawArrays( GL_TRIANGLES, 0, vcount );
  }

  return self;
}

mrb_value 
mrb_font_buffer_calculate_size(mrb_state* mrb, mrb_value self) 
{
  char* text;
  struct mrb_font_buffer* font_buffer = mrb_font_buffer_get_ptr(mrb, self);

  mrb_value text_value;
  mrb_get_args(mrb, "S", &text_value);
  text = mrb_string_value_ptr(mrb, text_value);

  vec2 pen = {{0,0}};
  size_t i;
  for( i=0; i<strlen(text); ++i )
  {
      texture_glyph_t *glyph = texture_font_get_glyph( font, text[i] );
      if( glyph != NULL )
      {
          int kerning = 0;
          if( i > 0)
          {
              kerning = texture_glyph_get_kerning( glyph, text[i-1] );
          }
          pen.x += kerning;
          pen.x += glyph->advance_x;
      }
  }

  return wrap_new_vec2(mrb, pen.x, pen.y);
}

void
init_mrb_font_buffer(mrb_state* mrb, struct RClass* mrb_freetype_class)
{
  mrb_font_buffer_class = mrb_define_class_under(mrb, mrb_freetype_class, "FontBuffer", mrb->object_class);
  MRB_SET_INSTANCE_TT(mrb_font_buffer_class, MRB_TT_DATA);

  mrb_define_class_method(mrb, mrb_font_buffer_class, "create", mrb_font_buffer_create, ARGS_REQ(2));
  
  mrb_define_method(mrb, mrb_font_buffer_class, "text=", mrb_font_buffer_set_text, ARGS_REQ(1));
  mrb_define_method(mrb, mrb_font_buffer_class, "calculate_size", mrb_font_buffer_calculate_size, ARGS_REQ(1));
  
  mrb_define_method(mrb, mrb_font_buffer_class, "bind", mrb_font_buffer_bind, ARGS_REQ(1));
  mrb_define_method(mrb, mrb_font_buffer_class, "unbind", mrb_font_buffer_unbind, ARGS_NONE());

  mrb_define_method(mrb, mrb_font_buffer_class, "render", mrb_font_buffer_render, ARGS_NONE());
}