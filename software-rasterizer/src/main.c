#include "memory_arena.h"
#include <math.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define mini(a, b) ((a) < (b) ? (a) : (b))
#define maxi(a, b) ((a) > (b) ? (a) : (b))
#define PI 3.14159265358979323846

typedef struct {
  float x, y, z;
} Point3D;

typedef struct {
  int x, y;
} Point2D;

typedef struct {
  int id;
  Point3D pos;
} Vertex;

typedef struct {
  Vertex *base;
  int len;
  int capacity;
} VertexArray;

typedef struct {
  int v1, v2, v3;
} Triangle;

typedef struct {
  Triangle *base;
  int len;
  int capacity;
} TriangleArray;

typedef struct {
  int width, height;
} Canvas;

const float ASPECT_RATIO = 1.0f;
const float FOV = 90.0f * (PI / 180.0f);

const Canvas CANVAS = {100, 100};

Point2D point_world_to_canvas(Point3D world_p) {
  assert(world_p.z > 0);
  float FOV_X = 2.0f * atan(tan(FOV / 2.0f) * ASPECT_RATIO);
  Point2D canvas_p = {0, 0};
  float x = world_p.x / (world_p.z * tan(FOV_X / 2.0f));
  float y = world_p.y / (world_p.z * tan(FOV / 2.0f));

  canvas_p.x = (int)((x + 1.0f) * 0.5f * CANVAS.width);
  canvas_p.y = (int)((1.0f - (y + 1.0f) * 0.5f) * CANVAS.height);
  return canvas_p;
}

// TODO manual pointer walk?
int parse_face_token(char *face_string) {
  char *context = NULL;
  char *vert = strtok_s(face_string, "/", &context);
  int parsed = atoi(vert);
  return parsed;
}

void triangle_push(TriangleArray *arr, Triangle new_tri) {
  assert(arr->len + 1 <= arr->capacity);
  arr->base[arr->len++] = new_tri;
}

void vertex_push(VertexArray *arr, Point3D new_vertex_p) {
  assert(arr->len + 1 <= arr->capacity);
  Vertex new_vertex = {0};
  new_vertex.id = arr->len;
  new_vertex.pos = new_vertex_p;
  arr->base[arr->len++] = new_vertex;
}

Vertex vertex_get(VertexArray arr, int index) {
  assert(index < arr.len);
  return arr.base[index];
}

Triangle triangle_get(TriangleArray arr, int index) {
  assert(index < arr.len);
  return arr.base[index];
}

int cross_product(Point2D a, Point2D b, Point2D c) {
  return (b.x - a.x) * (c.y - a.y) - (b.y - a.y) * (c.x - a.x);
}

bool is_point_in_tri(Point2D point, TriangleArray tri_arr,
                     VertexArray vert_arr) {
  for (int i = 0; i < tri_arr.len; i++) {
    Triangle tri = tri_arr.base[i];
    Point3D tri_world_p_1 = vert_arr.base[tri.v1].pos;
    if (tri_world_p_1.z <= 0)
      continue;
    Point2D tri_canvas_p_1 = point_world_to_canvas(tri_world_p_1);
    Point3D tri_world_p_2 = vert_arr.base[tri.v2].pos;
    if (tri_world_p_2.z <= 0)
      continue;
    Point2D tri_canvas_p_2 = point_world_to_canvas(tri_world_p_2);
    Point3D tri_world_p_3 = vert_arr.base[tri.v3].pos;
    if (tri_world_p_3.z <= 0)
      continue;
    Point2D tri_canvas_p_3 = point_world_to_canvas(tri_world_p_3);
    int min_x =
        mini(tri_canvas_p_1.x, mini(tri_canvas_p_2.x, tri_canvas_p_3.x));
    if (point.x < min_x)
      continue;
    int max_x =
        maxi(tri_canvas_p_1.x, maxi(tri_canvas_p_2.x, tri_canvas_p_3.x));
    if (point.x > max_x)
      continue;
    int min_y =
        mini(tri_canvas_p_1.y, mini(tri_canvas_p_2.y, tri_canvas_p_3.y));
    if (point.y < min_y)
      continue;
    int max_y =
        maxi(tri_canvas_p_1.y, maxi(tri_canvas_p_2.y, tri_canvas_p_3.y));
    if (point.y > max_y)
      continue;
    bool has_neg, has_pos;
    int winding_order =
        cross_product(tri_canvas_p_1, tri_canvas_p_2, tri_canvas_p_3);
    int edge_sign_1 = cross_product(tri_canvas_p_1, tri_canvas_p_2, point);
    int edge_sign_2 = cross_product(tri_canvas_p_2, tri_canvas_p_3, point);
    int edge_sign_3 = cross_product(tri_canvas_p_3, tri_canvas_p_1, point);

    has_neg = (edge_sign_1 < 0) || (edge_sign_2 < 0) || (edge_sign_3 < 0);
    has_pos = (edge_sign_1 > 0) || (edge_sign_2 > 0) || (edge_sign_3 > 0);

    bool is_inside = !(has_neg && has_pos);
    if (is_inside)
      return true;
  }
  return false;
}

const int FILE_LENGTH = 100;
const int CUBE_TRANSLATION[3] = {0, 0, 3};
const float CUBE_ROTATION_Y = (PI / 4);

void parse_file(VertexArray *vert_arr, TriangleArray *tri_arr) {
  FILE *fptr;
  errno_t err = fopen_s(&fptr, "cube.obj", "r");
  char fileString[FILE_LENGTH];
  fgets(fileString, FILE_LENGTH, fptr);

  while (fgets(fileString, FILE_LENGTH, fptr)) {
    if (fileString[0] == 'v' && fileString[1] == ' ') {
      char *context = NULL;
      char *type = strtok_s(fileString, " ", &context);
      char *v_x = strtok_s(NULL, " ", &context);
      char *v_y = strtok_s(NULL, " ", &context);
      char *v_z = strtok_s(NULL, " ", &context);
      float x = atof(v_x);
      float y = atof(v_y);
      float z = atof(v_z);

      float angle = CUBE_ROTATION_Y;
      float cos_theta = cosf(angle);
      float sin_theta = sinf(angle);

      float rotated_x = cos_theta * x - sin_theta * z;
      float rotated_y = y;
      float rotated_z = sin_theta * x + cos_theta * z;

      rotated_x += CUBE_TRANSLATION[0];
      rotated_y += CUBE_TRANSLATION[1];
      rotated_z += CUBE_TRANSLATION[2];

      Point3D vertex_p = {rotated_x, rotated_y, rotated_z};
      vertex_push(vert_arr, vertex_p);
    } else if (fileString[0] == 'f') {
      char *context = NULL;
      char *type = strtok_s(fileString, " ", &context);
      char *edge_1_string = strtok_s(NULL, " ", &context);
      int vert_1 = parse_face_token(edge_1_string);
      char *edge_2_string = strtok_s(NULL, " ", &context);
      int vert_2 = parse_face_token(edge_2_string);
      char *edge_3_string = strtok_s(NULL, " ", &context);
      int vert_3 = parse_face_token(edge_3_string);
      // Obj ids are 1 indexed
      Triangle tri = {vert_1 - 1, vert_2 - 1, vert_3 - 1};
      triangle_push(tri_arr, tri);
    }
  }
  fclose(fptr);
}

int main() {
  // Allocate memory_arena
  MemoryArena arena = {0};
  arena_init(&arena, 8 * 1024 * 1024);

  // Parse .obj into arrays
  VertexArray v_arr = {0};
  v_arr.capacity = 100;
  v_arr.base = arena_alloc(&arena, v_arr.capacity * sizeof(Vertex));

  TriangleArray t_arr = {0};
  t_arr.capacity = 100;
  t_arr.base = arena_alloc(&arena, t_arr.capacity * sizeof(Triangle));

  parse_file(&v_arr, &t_arr);

  // Print vertices
  // printf("Vertices: ");
  // for (int i = 0; i < v_arr.len; i++) {
  //   Vertex v = vertex_get(v_arr, i);
  //   printf("x: %f ", v.pos.x);
  //   printf("y: %f ", v.pos.y);
  //   printf("z: %f, ", v.pos.z);
  // }

  // Print tris
  // printf("Triangles: ");
  // for (int i = 0; i < t_arr.len; i++) {
  //   Triangle t = triangle_get(t_arr, i);
  //   printf("%i/", t.v1);
  //   printf("%i/", t.v2);
  //   printf("%i ", t.v3);
  // }

  // Step through each pixel in the canvas and fill in if its in a triangle
  FILE *fptr;
  errno_t err = fopen_s(&fptr, "test.ppm", "w");
  fprintf(fptr, "P3\n");
  fprintf(fptr, "%i %i\n", CANVAS.width, CANVAS.height);
  fprintf(fptr, "255\n");

  for (int y = 0; y < CANVAS.height; y++) {
    for (int x = 0; x < CANVAS.width; x++) {
      Point2D current_pixel = {x, y};
      int is_in_tri = is_point_in_tri(current_pixel, t_arr, v_arr);
      if (is_in_tri) {
        fprintf(fptr, "%i %i %i\n", 0, 0, 0);
      } else {
        fprintf(fptr, "%i %i %i\n", 255, 255, 255);
      }
    }
  }

  fclose(fptr);
}
