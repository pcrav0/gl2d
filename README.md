# how to run

```console
$ ./run.sh
```

# lib

```c
// modify the color of the items in the screen
Color color_of(byte r, byte g, byte b, byte a);
Color color_alpha(Color color, byte alpha);
void color_set(Color color);
Color color_hsv(f64 hue, f64 saturation, f64 value, f64 alpha);

// png textures
Texture texture_load_from_path(const char *path);

// vector 2d of float32
Vec2 vec2_of(f32 x, f32 y);
Vec2 vec2_scale(Vec2 self, f32 n);
Vec2 vec2_translate(Vec2 self, f32 n);
Vec2 vec2_add(Vec2 self, Vec2 other);
Vec2 vec2_mul(Vec2 self, Vec2 other);
void vec2_vertex(Vec2 self);

// rectangle for the screen position
Rectangle rectangle_of(f32 x, f32 y, f32 w, f32 h);
Rectangle rectangle_add(Rectangle self, Rectangle other);

// draw procedures
void triangle(Vec2 a, Vec2 b, Vec2 c, Color color);
void rectangle(Rectangle rect, Color color);
void polygon(Vec2s_View points, Color color);
void texture(const char *texture_path,
             Rectangle rect, Rectangle source,
             Color color);
void circle(Vec2 center, f64 radius, u8 resolution, Color color);
void character(char c, Rectangle rect, Color color);
void text(Strv str, Rectangle rect, Color color);

```
