/*
 * tools.c
 *
 *  Created on: 30/12/2014
 *      Author: chronno
 */
#include "tools.h"
#include <stdlib.h>
#include <string.h>
#include "mrgsl.h"
#include <mruby.h>
#include <mruby/value.h>
#include <mruby/variable.h>
#include <mruby/data.h>
#include <mruby/numeric.h>

void
mrb_attr_reader (mrb_state* mrb, struct RClass* type, const char* varname)
{
  mrb_value classobj;
  mrb_value sym;
  classobj = mrb_obj_value (type);
  sym = mrb_str_new_cstr (mrb, varname);
  mrb_funcall (mrb, classobj, "attr_reader", 1, sym);
}

void
mrb_attr_writer (mrb_state* mrb, struct RClass* type, const char* varname)
{
  mrb_value classobj;
  mrb_value sym;
  classobj = mrb_obj_value (type);
  sym = mrb_str_new_cstr (mrb, varname);
  mrb_funcall (mrb, classobj, "attr_writer", 1, sym);
}

void
mrb_attr_accessor (mrb_state* mrb, struct RClass* type, const char* varname)
{
  mrb_value classobj;
  mrb_value sym;
  classobj = mrb_obj_value (type);
  sym = mrb_str_new_cstr (mrb, varname);
  mrb_funcall (mrb, classobj, "attr_accessor", 1, sym);
}

void
mrb_set_gv (mrb_state* mrb, const char *name, mrb_value val)
{
  mrb_gv_set (mrb, mrb_intern_static (mrb, name, strlen (name)), val);
}

mrb_value
mrb_get_gv (mrb_state* mrb, const char *name)
{
  return mrb_gv_get (mrb, mrb_intern_static (mrb, name, strlen (name)));
}


void
mrb_set_iv (mrb_state* mrb, mrb_value object, const char *name, mrb_value val)
{
  mrb_iv_set (mrb, object, mrb_intern_static (mrb, name, strlen (name)), val);
}

mrb_value
mrb_get_iv (mrb_state* mrb, mrb_value object, const char *name)
{
  return mrb_iv_get (mrb, object, mrb_intern_static (mrb, name, strlen (name)));
}

mrb_bool
mrb_is_equals (mrb_state* mrb, mrb_value object, mrb_value other)
{
  return mrb_equal (mrb, object, other);
}

mrb_bool
mrb_is_a (mrb_state* mrb, mrb_value object, const char* classname)
{
  struct RClass* mrgsl = mrb_module_get(mrb, "MRGSL");
  struct RClass* class = mrb_class_get_under (mrb,mrgsl ,classname);
  return mrb_obj_is_instance_of (mrb, object, class);
}

mrb_value
mrb_new_instance (mrb_state* mrb, const char* class, mrb_int argc, ...)
{
  va_list arguments;
  mrb_value* params;
  int i;
  struct RClass *mrgsl;
  struct RClass *type;
  if (argc > 0)
    {
      params = malloc (sizeof(mrb_value) * argc);

      va_start(arguments, argc);
      for (i = 0; i < argc; i++)
	{
	  params[i] = va_arg(arguments, mrb_value);
	}
      va_end(arguments);
    }
  else
    {
      params = NULL;
    }
  mrgsl = mrb_module_get(mrb, "MRGSL");
  type = mrb_class_get_under(mrb, mrgsl,class);
  return mrb_obj_new (mrb, type, argc, params);
}

void
mrgsl_viewport_add_child (mrb_value parent, mrb_value child)
{

  mrgsl_viewport* viewport = (mrgsl_viewport*) DATA_PTR(parent);
  if (viewport->size >= viewport->capacity)
    {
      viewport->capacity = viewport->capacity * 2 + 1;
      viewport->children = realloc (viewport->children, sizeof(mrb_value*) * viewport->capacity);
    }

  viewport->children[viewport->size] = child;
  viewport->size += 1;
  //TODO SORT
}

void
mrgsl_viewport_remove_child (mrb_state* mrb, mrb_value parent, mrb_value child)
{
  mrgsl_viewport* viewport = (mrgsl_viewport*) DATA_PTR(parent);
  for (int i = viewport->size - 1; i >= 0; --i)
    {
      if (mrb_is_equals (mrb, viewport->children[i], child))
	{
	  viewport->size -= 1;
	  viewport->children[i] = mrb_nil_value ();
	  for (int t = i; t < viewport->size; t++)
	    {
	      viewport->children[t] = viewport->children[t + 1];
	    }
	}
    }
}

void
mrgsl_draw_viewport (mrb_state* mrb, mrb_value viewport)
{
  mrgsl_viewport* struc = DATA_PTR(viewport);
  for (int i = 0; i < struc->size; i++)
    {
      mrb_value child = struc->children[i];
      mrb_value visible = mrb_get_iv (mrb, child, "@visible");
      if (mrb_is_equals (mrb, visible, mrb_true_value ()))
	{
	  if (mrb_is_a (mrb, child, "Viewport"))
	    {
	      mrgsl_draw_viewport (mrb, child);
	    }
	  else if (mrb_is_a (mrb, child, "Sprite"))
	    {
	      mrgsl_draw_sprite (mrb, child);
	    }
	}
    }
}

mrb_value get_graphics_viewport(mrb_state* mrb ){
  mrb_value graphics =mrb_obj_value(mrb_module_get_under(mrb, mruby_get_mrgsl(mrb), "Graphics"));
  return mrb_get_iv(mrb, graphics,"@viewport");
}

void
mrgsl_draw_sprite (mrb_state* mrb, mrb_value sprite)
{
  mrb_value rect;
  mrb_value mrb_bitmap = mrb_get_iv (mrb, sprite, "@bitmap");
  mrgsl_bitmap* bitmap = (mrgsl_bitmap*) DATA_PTR(mrb_bitmap);
  mrb_int rect_x;
  mrb_int rect_y;
  mrb_int rect_w;
  mrb_int rect_h;
  mrb_int spr_x;
  mrb_int spr_y;
  mrb_value parent;
  mrb_value prect;
  mrb_int view_x;
  mrb_int view_y;
  float rx;
  float ry;
  float rw;
  float rh;
  if (bitmap == NULL)
    {
      return;
    }
  rect = mrb_get_iv (mrb, sprite, "@rect");
  if (mrb_is_equals (mrb, rect, mrb_nil_value ()))
    {
      mrb_value nrect = mrb_new_instance (mrb, "Rect", 4, 0, 0, bitmap->surface->w, bitmap->surface->h);
      rect = nrect;
      mrb_set_iv (mrb, sprite, "@rect", nrect);
    }
  glBindTexture ( GL_TEXTURE_2D, bitmap->texture);

  /*
   * rect values
   */
  rect_x = mrb_int(mrb, mrb_get_iv (mrb,rect, "@x"));
  rect_y = mrb_int(mrb, mrb_get_iv (mrb,rect, "@y"));
  rect_w = mrb_int(mrb,  mrb_get_iv (mrb,rect, "@width"));
  rect_h = mrb_int(mrb, mrb_get_iv (mrb,rect, "@height"));
  /*
   * sprite values
   */
  spr_x = mrb_int (mrb, mrb_get_iv (mrb,sprite, "@x"));
  spr_y = mrb_int (mrb, mrb_get_iv (mrb, sprite, "@y"));
  /*
   * Parent value
   */

  parent = mrb_get_iv (mrb, sprite, "@parent");
  prect = mrb_get_iv (mrb, parent, "@rect");
  view_x = mrb_int (mrb, mrb_get_iv (mrb, prect, "@x"));
  view_y = mrb_int (mrb, mrb_get_iv (mrb, prect, "@y"));

  rx = rect_x / (float) bitmap->surface->w;
  ry = rect_y / (float) bitmap->surface->h;
  rw = rect_w / (float) bitmap->surface->w;
  rh = rect_h / (float) bitmap->surface->h;

  glLoadIdentity ();
  glTranslatef (spr_x + view_x, spr_y + view_y, 0);
  glEnable (GL_TEXTURE_2D);
  glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

  glBegin ( GL_QUADS);
  //top-left vertex (corner)
  glTexCoord2i (rx, ry);
  glVertex3f (spr_x, spr_y, 0.f);

  //top-right vertex (corner)
  glTexCoord2f (rx + rw, ry);
  glVertex3f (spr_x + rect_w, spr_y, 0.f);

  //bottom-right vertex (corner)
  glTexCoord2f (rx + rw, ry + rh);
  glVertex3f (spr_x + rect_w, spr_y + rect_h, 0.f);

  //bottom-left vertex (corner)
  glTexCoord2f (rx, ry + rh);
  glVertex3f (spr_x, spr_y + rect_h, 0.f);

  glEnd ();
}
