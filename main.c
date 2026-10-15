#include <GLFW/glfw3.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#define LIB_IMPLEMENTATION
#include "./c/build/lib.h"
#include "stb_image.h"

/// utils


#define TAU ((f64)(M_PI*2))

static bool streq(const char **a, const char **b) {
	if (*a == *b) return true;
	if (!*a || !*b) return false;
	return strcmp(*a, *b) == 0;
}

/// globals

Allocator al;

/// color


typedef union {
	struct { byte r, g, b, a; };
	struct { byte x, y, z, w; };
	u32 number;
	u8 bytes[4];
} Color;

#define WHITE ((Color){0xff, 0xff, 0xff, 0xff})
#define BLACK ((Color){0, 0, 0, 0xff})
#define RED ((Color){0xff, 0, 0, 0xff})
#define GREEN ((Color){0, 0xff, 0, 0xff})
#define BLUE ((Color){0, 0, 0xff, 0xff})
#define YELLOW ((Color){0xff, 0xff, 0, 0xff})

static_assert(sizeof(Color) == sizeof(u32));

Color color_of(byte r, byte g, byte b, byte a) {
	return (Color){ .r = r, .g = g, .b = b, .a = a, };
}

Color color_alpha(Color color, byte alpha) {
	return color_of(color.r, color.g, color.b, alpha);
}

void color_set(Color color) {
	byte n = type_max_u(byte);
	glColor4f((f64)color.r / n, (f64)color.g / n, (f64)color.b / n,
	          (f64)color.a / n);
}

// from raylib.h
Color color_hsv(f64 hue, f64 saturation, f64 value, f64 alpha) {
	Color result = { 0, 0, 0, 0xff * alpha };
	hue *= 6;

	{
		f64 k = f64_mod(5.0 + hue, 6);
		f64 t = 4.0 - k;
		k = t < k ? t : k;
		k = k < 1 ? k : 1;
		k = k > 0 ? k : 0;
		result.r = (byte)((value - value*saturation*k)*(f64)0xff);
	}

	{
		f64 k = f64_mod(3.0 + hue, 6);
		f64 t = 4.0 - k;
		k = t < k ? t : k;
		k = k < 1 ? k : 1;
		k = k > 0 ? k : 0;
		result.g = (byte)((value - value*saturation*k)*(f64)0xff);
	}

	{
		f64 k = f64_mod(1.0 + hue, 6);
		f64 t = 4.0 - k;
		k = t < k ? t : k;
		k = k < 1 ? k : 1;
		k = k > 0 ? k : 0;
		result.b = (byte)((value - value*saturation*k)*(f64)0xff);
	}

	return result;
}

/// texture


typedef struct {
	int id;
	byte *data;
	u16 width;
	u16 height;
} Texture;

typedef struct {
	struct { const char *key; Texture value; } *items; // path -> texture
	usize length;
	usize capacity;
	Eq *eq;
} Texture_Cache;

static_assert(assoc_type(Texture_Cache), "");

static Texture_Cache texture_cache = { .eq = (void*)streq };

Texture texture_load_from_path(const char *path) {
	{ // check if in cache
		Texture *result = assoc_get(&texture_cache, path);
		if (result)
			return *result;
	}

	int width, height, channels;
	byte *data = stbi_load(path, &width, &height, &channels, STBI_rgb_alpha);
	assert(channels == 4);

	uint id;
	glGenTextures(1, &id);
	glBindTexture(GL_TEXTURE_2D, id);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA,
	             GL_UNSIGNED_BYTE, data);

	Texture result = {
		.id = id, .data = data,
		.width = width, .height = height,
	};
	assoc_put(&texture_cache, path, result, &al);

	return result;
}

/// vec


typedef struct {
	f32 x, y;
} Vec2;

typedef struct {
	Vec2 *items;
	usize length;
} Vec2s_View;

typedef struct {
	union {
		struct {
			Vec2 *items;
			usize length;
		};
		Vec2s_View slice;
	};
	usize capacity;
} Vec2s;

Vec2 vec2_of(f32 x, f32 y) {
	return (Vec2){ .x = x, .y = y, };
}

Vec2 vec2_scale(Vec2 self, f32 n) {
	return vec2_of(self.x * n, self.y * n);
}

Vec2 vec2_translate(Vec2 self, f32 n) {
	return vec2_of(self.x + n, self.y + n);
}

Vec2 vec2_add(Vec2 self, Vec2 other) {
	return vec2_of(self.x + other.x, self.y + other.y);
}

Vec2 vec2_mul(Vec2 self, Vec2 other) {
	return vec2_of(self.x * other.x, self.y * other.y);
}

void vec2_vertex(Vec2 self) {
	Vec2 result = self;                      // 0..1
	result = vec2_scale(result, 2);          // 0..2
	result = vec2_add(result, vec2_of(-1, -1)); // -1..1
	result = vec2_mul(result, vec2_of(1, -1));  // invert y
	glVertex2f(result.x, result.y);
}

/// lib


typedef union {
	struct { Vec2 position, scale; };
	struct { f32 x, y, w, h; };
} Rectangle;

static_assert(sizeof(Rectangle) == sizeof(Vec2) * 2);

Rectangle rectangle_of(f32 x, f32 y, f32 w, f32 h) {
	return (Rectangle){ .x = x, .y = y, .w = w, .h = h };
}

Rectangle rectangle_add(Rectangle self, Rectangle other) {
	return rectangle_of(
		self.x + other.x,
		self.y + other.y,
		self.w + other.w,
		self.h + other.h
	);
}

void triangle(Vec2 a, Vec2 b, Vec2 c, Color color) {
	color_set(color);

	glBegin(GL_TRIANGLES);

	vec2_vertex(a);
	vec2_vertex(b);
	vec2_vertex(c);

	glEnd();
}

void rectangle(Rectangle rect, Color color) {
	color_set(color);

	glBegin(GL_QUADS);

	vec2_vertex(rect.position);
	vec2_vertex(vec2_add(rect.position, vec2_of(rect.scale.x, 0)));
	vec2_vertex(vec2_add(rect.position, rect.scale));
	vec2_vertex(vec2_add(rect.position, vec2_of(0, rect.scale.y)));

	glEnd();
}

void polygon(Vec2s_View points, Color color) {
	color_set(color);

	glBegin(GL_POLYGON);

	for (usize i = 0; i < points.length; i++)
		vec2_vertex(*slice_at(&points, i));

	glEnd();
}

void texture(const char *texture_path,
             Rectangle rect, Rectangle source,
             Color color) {
	Texture texture = texture_load_from_path(texture_path);

	glBindTexture(GL_TEXTURE_2D, texture.id);
	color_set(color);

	glBegin(GL_QUADS);

	glTexCoord2f(source.x, source.y);
	vec2_vertex(rect.position);
	glTexCoord2f(source.w, source.y);
	vec2_vertex(vec2_add(rect.position, vec2_of(rect.scale.x, 0)));
	glTexCoord2f(source.w, source.h);
	vec2_vertex(vec2_add(rect.position, rect.scale));
	glTexCoord2f(source.x, source.h);
	vec2_vertex(vec2_add(rect.position, vec2_of(0, rect.scale.y)));

	glEnd();

	glBindTexture(GL_TEXTURE_2D, 0);
}

// https://www.reddit.com/r/opengl/comments/6f4q7t/how_do_you_draw_a_circle_in_modern_opengl
void circle(Vec2 center, f64 radius, u8 resolution, Color color) {
	color_set(color);

	glBegin(GL_POLYGON);

	for (u8 i = 0; i < resolution; i++) {
		f64 n = (f64)i / resolution * TAU;
		vec2_vertex(vec2_of(center.x + radius * cos(n),
		                    center.y + radius * sin(n)));
	}

	glEnd();
}

void character(char c, Rectangle rect, Color color) {
	Texture t = texture_load_from_path("./text.png");

	u64 sw = 5; // width in pixels per char
	u64 sh = 8; // heigth in pixels per char

	u64 w = t.width / sw;
	u64 h = t.height / sh;

	u64 x = (c % w) * sw;
	u64 y = (c / h) * sh;

	texture(
		"./text.png",
		rect,
		rectangle_of(
			(f64)x / t.width,
			(f64)y / t.height,
			(f64)(x + sw) / t.width,
			(f64)(y + sh) / t.height
		),
		color
	);
}

void text(Strv str, Rectangle rect, Color color) {
	Vec2 scale_item = vec2_scale(rect.scale, 1. / str.length);

	for (usize i = 0; i < str.length; i++) {
		char it = *slice_at(&str, i);
		Rectangle r = {
			.position = vec2_add(rect.position, vec2_of(i * scale_item.x, 0)),
			.scale = scale_item,
		};
		character(
			it,
			r,
			color
		);
	}
}

/// impl


int main(void) {
	al = al_stdlib_create();

	GLFWwindow* window;

	if (!glfwInit())
		return -1;

	window = glfwCreateWindow(640, 480, "open gl #DEBUG", NULL, NULL);
	if (!window) {
		glfwTerminate();
		return -1;
	}

	glfwMakeContextCurrent(window);

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_TEXTURE_2D);

	glBindTexture(GL_TEXTURE_2D, 0);

	f64 triangle_hue = 0;
	f64 mole_value = 0;

	while (!glfwWindowShouldClose(window)) {
		static f64 last_time = 0.0;

		f64 current_time = glfwGetTime();
		f64 delta_time = current_time - last_time;

		triangle_hue += 0.2 * delta_time;
		mole_value += 0.3 * delta_time;

		glClearColor(0.0, 0.0, 0.0, 0.0);
		glClear(GL_COLOR_BUFFER_BIT);

		if (false) {
			Rectangle source_all = rectangle_of(0, 0, 1, 1);
			circle(vec2_of(.5, .5), 0.2, 30, RED);

			texture("./bomb.png", rectangle_of(1.05, .5, -.5, .5), source_all, WHITE);
			texture("./mole.png", rectangle_of(.7, .1, -.2, .2), source_all,
			        color_hsv(0, mole_value, 1, 1));
			polygon(
				SLICE_OF(Vec2s_View,
				         vec2_of(.5, .2),
				         vec2_of(.6, .25),
				         vec2_of(.7, .5),
				         vec2_of(.8, .8),
				         vec2_of(.4, .9),
				         vec2_of(.3, .7)),
				color_hsv(triangle_hue, 1, .8, .4)
			);

			triangle(vec2_of(.4, 0), vec2_of(.4, .5), vec2_of(.2, .5),
			         color_alpha(YELLOW, 0xff*.5));
			rectangle(rectangle_of(.1, .05, .2, .6), color_alpha(BLUE, 0xff*.3));

			rectangle(rectangle_of(.7, .05, .2, .6), color_alpha(BLUE, 0xff*.3));
			triangle(vec2_of(.6, 0), vec2_of(.8, .5), vec2_of(.6, .5),
			         color_alpha(YELLOW, 0xff*.5));

			text(STRV("ola"), rectangle_of(.04, 0, .3, .3), WHITE);
			text(STRV("mundo"), rectangle_of(.04, .1, .4, .4), WHITE);
			text(STRV("ABCD"), rectangle_of(.04, .2, .95, 1), WHITE);
		}

		text(STRV(":)"), rectangle_of(.7, .05, .2, .6), WHITE);

		glFlush();
		glfwSwapBuffers(window);
		glfwPollEvents();

		last_time = current_time;
	}

	glfwTerminate();

	// cleanup
	for (usize i = 0; i < texture_cache.length; i++) {
		Texture it = texture_cache.items[i].value;
		glDeleteTextures(1, &it.id);
		stbi_image_free(it.data);
	}

	al_stdlib_destroy(al.context);
	return 0;
}
