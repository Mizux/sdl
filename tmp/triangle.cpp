#include <exception>
#include <functional>

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif

#define GL_GLEXT_PROTOTYPES 1
#define EGL_EGLEXT_PROTOTYPES
#include <SDL.h>
#include <SDL_opengles2.h>

// Vertex shader
GLint         shaderPan, shaderZoom, shaderAspect;
const GLchar* vertexSource =
    "uniform vec2 pan;                             \n"
    "uniform float zoom;                           \n"
    "uniform float aspect;                         \n"
    "attribute vec4 position;                      \n"
    "varying vec3 color;                           \n"
    "void main()                                   \n"
    "{                                             \n"
    "    gl_Position = vec4(position.xyz, 1.0);    \n"
    "    gl_Position.xy += pan;                    \n"
    "    gl_Position.xy *= zoom;                   \n"
    "    gl_Position.y *= aspect;                  \n"
    "    color = gl_Position.xyz + vec3(0.5);      \n"
    "}                                             \n";

// Fragment/pixel shader
const GLchar* fragmentSource =
    "precision mediump float;                     \n"
    "varying vec3 color;                          \n"
    "void main()                                  \n"
    "{                                            \n"
    "    gl_FragColor = vec4 ( color, 1.0 );      \n"
    "}                                            \n";

std::function<void()> loop;
void                  mainLoop() {
  loop();
}

void updateShader() {
  // glUniform2fv(shaderPan, 1, camera.pan());
  // glUniform1f(shaderZoom, camera.zoom());
  // glUniform1f(shaderAspect, camera.aspect());
}

GLuint initShader() {
  // Create and compile vertex shader
  GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
  glShaderSource(vertexShader, 1, &vertexSource, NULL);
  glCompileShader(vertexShader);

  // Create and compile fragment shader
  GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
  glShaderSource(fragmentShader, 1, &fragmentSource, NULL);
  glCompileShader(fragmentShader);

  // Link vertex and fragment shader into shader program and use it
  GLuint shaderProgram = glCreateProgram();
  glAttachShader(shaderProgram, vertexShader);
  glAttachShader(shaderProgram, fragmentShader);
  glLinkProgram(shaderProgram);
  glUseProgram(shaderProgram);

  // Get shader variables and initialize them
  shaderPan    = glGetUniformLocation(shaderProgram, "pan");
  shaderZoom   = glGetUniformLocation(shaderProgram, "zoom");
  shaderAspect = glGetUniformLocation(shaderProgram, "aspect");
  updateShader();

  return shaderProgram;
}

void initGeometry(GLuint shaderProgram) {
  // Create vertex buffer object and copy vertex data into it
  GLuint vbo;
  glGenBuffers(1, &vbo);
  glBindBuffer(GL_ARRAY_BUFFER, vbo);
  GLfloat vertices[] = {0.0f, 0.5f, 0.0f, -0.5f, -0.5f, 0.0f, 0.5f, -0.5f, 0.0f};
  glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

  // Specify the layout of the shader vertex data (positions only, 3 floats)
  GLint posAttrib = glGetAttribLocation(shaderProgram, "position");
  glEnableVertexAttribArray(posAttrib);
  glVertexAttribPointer(posAttrib, 3, GL_FLOAT, GL_FALSE, 0, 0);
}

int main(int argc, char* argv[]) {
  if (SDL_InitSubSystem(SDL_INIT_VIDEO) != 0) {
    SDL_Log("Unable to initialize SDL video subsystem: %s\n", SDL_GetError());
    return 1;
  }

  // ref: https://wiki.libsdl.org/SDL2/SDL_GL_SetAttribute
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
  SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
  SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);

  SDL_Window* window = SDL_CreateWindow(
      "mizux",
      SDL_WINDOWPOS_CENTERED,
      SDL_WINDOWPOS_CENTERED,
      640,
      480,
      SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN);
  if (window == nullptr) {
    SDL_Log("Unable to create window: %s", SDL_GetError());
    return 1;
  }

  SDL_GLContext gl_context = SDL_GL_CreateContext(window);
  if (gl_context == NULL) {
    SDL_Log("Unable to create GL context: %s", SDL_GetError());
    return 1;
  }

  SDL_GL_SetSwapInterval(0);

  //auto rdr = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_TARGETTEXTURE);

  // Initialize shader and geometry
  GLuint shaderProgram = initShader();
  initGeometry(shaderProgram);

  SDL_Event e;

  loop = [&] {
    updateShader();

    while (SDL_PollEvent(&e)) {
      if (e.type == SDL_QUIT)
        std::terminate();
    }

    // Clear screen
    glClear(GL_COLOR_BUFFER_BIT);
    // Draw the vertex buffer
    glDrawArrays(GL_TRIANGLES, 0, 3);

    SDL_GL_SwapWindow(window);
  };

  // Start the main loop
#ifdef __EMSCRIPTEN__
  int fps = 0; // Use browser's requestAnimationFrame
  emscripten_set_main_loop(mainLoop, fps, true);
#else
  while (true) {
    mainLoop();
  }
#endif

  return 0;
}
