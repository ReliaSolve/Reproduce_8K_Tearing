#include <iostream>
#include <string>
#include <chrono>
#include <vector>
#include <array>
#include <memory>
#include <cmath>
#include <GL/glew.h>
#include <GLFW/glfw3.h>

//================================================================================================
// Vertex and fragment shader source code and functions to check for errors when building them.

static const GLchar* VertexShader =
R"(#version 330 core
   layout(location = 0) in vec3 position;
   layout(location = 1) in vec3 vertexColor;
   out vec3 fragmentColor;
   uniform mat4 modelViewProjection;
   void main()
   {
      gl_Position = modelViewProjection * vec4(position,1);
      fragmentColor = vertexColor;
   })";

static const GLchar* FragmentShader =
R"(#version 330 core
   in vec3 fragmentColor;
   out vec3 color;
   void main()
   {
       color = fragmentColor;
   })";

void checkShaderError(GLuint shaderId, const std::string& exceptionMsg) {
  GLint result = GL_FALSE;
  int infoLength = 0;
  glGetShaderiv(shaderId, GL_COMPILE_STATUS, &result);
  glGetShaderiv(shaderId, GL_INFO_LOG_LENGTH, &infoLength);
  if (result == GL_FALSE) {
    std::vector<GLchar> errorMessage(infoLength + 1);
    glGetShaderInfoLog(shaderId, infoLength, NULL, &errorMessage[0]);
    std::cerr << &errorMessage[0] << std::endl;
    throw std::runtime_error(exceptionMsg);
  }
}

void checkProgramError(GLuint programId, const std::string& exceptionMsg) {
  GLint result = GL_FALSE;
  int infoLength = 0;
  glGetProgramiv(programId, GL_LINK_STATUS, &result);
  glGetProgramiv(programId, GL_INFO_LOG_LENGTH, &infoLength);
  if (result == GL_FALSE) {
    std::vector<GLchar> errorMessage(infoLength + 1);
    glGetProgramInfoLog(programId, infoLength, NULL, &errorMessage[0]);
    std::cerr << &errorMessage[0] << std::endl;
    throw std::runtime_error(exceptionMsg);
  }
}

//================================================================================================
// Class to generate and draw colored geometry with internal patches.

class MeshPlane {
public:
  MeshPlane(GLfloat scale, size_t numTriangles = 2 * 15 * 15, std::array<float,3> color = {1, 1, 1}) {
    // Figure out how many quads we have per edge.  There
    // is a minimum of 1.
    size_t numQuads = numTriangles / 2;
    size_t numQuadsPerEdge = static_cast<size_t> (sqrt(numQuads));
    if (numQuadsPerEdge < 1) { numQuadsPerEdge = 1; }

    // Construct a square with the specified number of
    // quads a plane in Z.
    for (size_t i = 0; i < numQuadsPerEdge; i++) {
      for (size_t j = 0; j < numQuadsPerEdge; j++) {

        // Modulate the brightness of each quad by a random luminance,
        // leaving all vertices the same hue.
        GLfloat brightness = 0.5f + rand() * 0.5f / RAND_MAX;
        const size_t numTris = 2;
        const size_t numColors = 3;
        const size_t numVerts = 3;
        for (size_t c = 0; c < numTris * numVerts; c++) {
          for (size_t i = 0; i < numColors; i++) {
            colorBufferData.push_back(brightness * color[i]);
          }
        }

        // Send the two triangles that make up this quad, where the
        // quad covers the appropriate fraction of the face from
        // -scale to scale in X and Y.
        GLfloat Z = 0.0f;
        GLfloat minX = -scale + i * (2 * scale) / numQuadsPerEdge;
        GLfloat maxX = -scale + (i + 1) * (2 * scale) / numQuadsPerEdge;
        GLfloat minY = -scale + j * (2 * scale) / numQuadsPerEdge;
        GLfloat maxY = -scale + (j + 1) * (2 * scale) / numQuadsPerEdge;
        vertexBufferData.push_back(minX);
        vertexBufferData.push_back(maxY);
        vertexBufferData.push_back(Z);

        vertexBufferData.push_back(minX);
        vertexBufferData.push_back(minY);
        vertexBufferData.push_back(Z);

        vertexBufferData.push_back(maxX);
        vertexBufferData.push_back(minY);
        vertexBufferData.push_back(Z);

        vertexBufferData.push_back(maxX);
        vertexBufferData.push_back(maxY);
        vertexBufferData.push_back(Z);

        vertexBufferData.push_back(minX);
        vertexBufferData.push_back(maxY);
        vertexBufferData.push_back(Z);

        vertexBufferData.push_back(maxX);
        vertexBufferData.push_back(minY);
        vertexBufferData.push_back(Z);
      }
    }
  }

  ~MeshPlane() {
    if (initialized) {
      glDeleteBuffers(1, &vertexBuffer);
      glDeleteBuffers(1, &colorBuffer);
    }
  }

  void init() {
    if (!initialized) {
      // Unbind any vertex array object.
      glBindVertexArray(0);

      // Vertex buffer
      glGenBuffers(1, &vertexBuffer);
      glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer);
      glBufferData(GL_ARRAY_BUFFER,
        sizeof(vertexBufferData[0]) * vertexBufferData.size(),
        vertexBufferData.data(), GL_STATIC_DRAW);
      glBindBuffer(GL_ARRAY_BUFFER, 0);

      // Color buffer
      glGenBuffers(1, &colorBuffer);
      glBindBuffer(GL_ARRAY_BUFFER, colorBuffer);
      glBufferData(GL_ARRAY_BUFFER,
        sizeof(colorBufferData[0]) * colorBufferData.size(),
        colorBufferData.data(), GL_STATIC_DRAW);
      glBindBuffer(GL_ARRAY_BUFFER, 0);

      initialized = true;
    }
  }

  void draw() {
    init();

    // Unbind any currently bound vertex array object.
    // We cannot use vertex array objects because we're potentially going to be called
    // from multiple OpenGL contexts in different threads and VAOs are not shared between
    // contexts.
    glBindVertexArray(0);

    // Enable the vertex attribute arrays we are going to use
    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);

    // Bind the vertex buffer object
    glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (GLvoid*)0);

    // Bind the color buffer object
    glBindBuffer(GL_ARRAY_BUFFER, colorBuffer);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, (GLvoid*)0);

    // Draw our geometry
    glDrawArrays(GL_TRIANGLES, 0, static_cast<GLsizei>(vertexBufferData.size()));
  }

private:
  MeshPlane(const MeshPlane&) = delete;
  MeshPlane& operator=(const MeshPlane&) = delete;
  bool initialized = false;
  GLuint colorBuffer = 0;
  GLuint vertexBuffer = 0;
  std::vector<GLfloat> colorBufferData;
  std::vector<GLfloat> vertexBufferData;
};

int main(int argc, char* argv[])
{
  // Value of -1 does not use full screen. Setting to 0 or higher selects the display to use.
  int fullScreenDisplay = 1;
  int width = 7680;
  int height = 4320;
  double fps = 60.0;

  for (int i = 1; i < argc; ++i) {
    std::string arg = argv[i];
    if (arg == "--fullScreenDisplay" && i + 1 < argc) {
      fullScreenDisplay = std::stoi(argv[++i]);
    } else if (arg == "--width" && i + 1 < argc) {
      width = std::stoi(argv[++i]);
    } else if (arg == "--height" && i + 1 < argc) {
      height = std::stoi(argv[++i]);
    } else if (arg == "--fps" && i + 1 < argc) {
      fps = std::stod(argv[++i]);
    }
  }

  std::cout << "FullScreen display (-1 for none): " << fullScreenDisplay << std::endl;

  glfwInit();

  // Tell it not to iconify full-screen windows that lose focus.
  glfwWindowHint(GLFW_AUTO_ICONIFY, GLFW_FALSE);

  // Create a windowed mode window and its OpenGL context.
  // This must be done in the same thread that will do the rendering so that the window events will
  // be handled properly on all architectures.
  // We must make the OpenGL context of the window we want to share current on this thread
  // if we are sharing it by borrowing it and then returning it once the window is open because
  // Windows requires it to be current.
  GLFWwindow* windowToShare = nullptr;
  GLFWwindow* m_window = nullptr;
  m_window = glfwCreateWindow(width, height, "Reproduce_8K_Tearing", nullptr, windowToShare);

  // Verify that the window was created.
  if (!m_window) {
    std::cerr << "Failed to create GLFW window" << std::endl;
    return 1;
  }

  // Determine the full-screen monitor to use, if any.
  GLFWmonitor* fullScreenMonitor = nullptr;
  if (fullScreenDisplay >= 0) {
    int count;
    GLFWmonitor** monitors = glfwGetMonitors(&count);
    if ((count == 0) || !monitors) {
      std::cerr << "No monitors for fullscreen" << std::endl;
      return 2;
    }
    if (fullScreenDisplay >= count) {
      std::cerr << "Invalid monitor requested (index larger than available monitors)" << std::endl;
      return 3;
    }
    fullScreenMonitor = monitors[fullScreenDisplay];
  }

  // If we're displaying full-screen engage that here along with specifying the refresh rate.
  if (fullScreenDisplay >= 0) {
    glfwSetWindowMonitor(m_window, fullScreenMonitor, 0, 0, width, height, fps);
  }

  // Make the window's context current
  glfwMakeContextCurrent(m_window);

  // Initialize GLEW in our context.
  glewExperimental = true;
  if (glewInit() != GLEW_OK) {
    std::cerr << "Failed to initialize GLEW" << std::endl;
    return 4;
  }
  // Clear any OpenGL error that Glew caused.  On Non-Windows platforms, this can cause a spurious error 1280.
  glGetError();

  //================================================================================================
  // Shaders and OpenGL program variables setup

  // Construct the shader programs.
  GLuint vertexShaderId = glCreateShader(GL_VERTEX_SHADER);
  GLuint fragmentShaderId = glCreateShader(GL_FRAGMENT_SHADER);

  // vertex shader
  glShaderSource(vertexShaderId, 1, &VertexShader, NULL);
  glCompileShader(vertexShaderId);
  checkShaderError(vertexShaderId, "Vertex shader compilation failed.");

  // fragment shader
  glShaderSource(fragmentShaderId, 1, &FragmentShader, NULL);
  glCompileShader(fragmentShaderId);
  checkShaderError(fragmentShaderId, "Fragment shader compilation failed.");

  // linking shader program
  GLuint programId = glCreateProgram();
  glAttachShader(programId, vertexShaderId);
  glAttachShader(programId, fragmentShaderId);
  glLinkProgram(programId);
  checkProgramError(programId, "Shader program link failed.");

  // once linked into a program, we no longer need the shaders.
  glDeleteShader(vertexShaderId);
  glDeleteShader(fragmentShaderId);

  GLuint modelViewProjectionUniformId = glGetUniformLocation(programId, "modelViewProjection");

  glUseProgram(programId);
  glDisable(GL_CULL_FACE);

  //================================================================================================
  // Make our geometry objects, which will know how to draw themselves.  There will be 21 of them with
  // colors chosen from a set of 6.
  float radius = 0.25f;
  size_t quadsPerEdge = 10;
  size_t trianglesPerSide = 2 * quadsPerEdge * quadsPerEdge;
  // 6 faces
  size_t numTriangles = static_cast<size_t>(trianglesPerSide * 6);
  std::vector< std::array<float, 3> > colors = {
    {1.0f, 0.5f, 0.5f},
    {0.5f, 1.0f, 0.5f},
    {0.5f, 0.5f, 1.0f},
    {1.0f, 1.0f, 0.5f},
    {0.5f, 1.0f, 1.0f},
    {1.0f, 0.5f, 1.0f}
  };
  std::vector< std::shared_ptr<MeshPlane> > planes;
  unsigned NX = 7;
  unsigned NY = 3;
  for (unsigned i = 0; i < NX; i++) {
    for (unsigned j = 0; j < NY; j++) {
      planes.push_back(std::shared_ptr<MeshPlane>(
        new MeshPlane(radius, numTriangles, colors[(i + j) % colors.size()])));
    }
  }

  //================================================================================================
  // Timing the main loop.

  std::chrono::steady_clock::time_point start = std::chrono::steady_clock::now();
  size_t count = 0;

  // Loop until the user closes the window using Alt-F4 or the close button.
  std::cout << "Use the OS-specific close button or full-screen quit (Alt-F4 or Apple-Q) to close the window." << std::endl;
  while (++count) {

    // Clear the screen
    glClearColor(0.6f, 0.8f, 1.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    /// @todo
    GLfloat viewProjection[16] = {
      1.0f, 0.0f, 0.0f, 0.0f,
      0.0f, 1.0f, 0.0f, 0.0f,
      0.0f, 0.0f, 1.0f, 0.0f,
      0.0f, 0.0f, 0.0f, 1.0f
    };
    glUniformMatrix4fv(modelViewProjectionUniformId, 1, GL_FALSE, viewProjection);
    for (auto& plane : planes) {
      plane->draw();
    }

    // Swap front and back buffers and wait for it to complete.
    glfwSwapBuffers(m_window);
    glFinish();

    // Poll for and process events, including window closure.
    glfwPollEvents();

    // Done when the user closes the window.
    if (glfwWindowShouldClose(m_window)) {
      std::cout << "Closing window" << std::endl;
      break;
    }
  }

  std::chrono::steady_clock::time_point stop = std::chrono::steady_clock::now();
  std::chrono::duration<double> elapsed = stop - start;
  std::cout << "Elapsed time: " << elapsed.count() << " seconds" << std::endl;
  std::cout << "Frames per second: " << count / elapsed.count() << std::endl;

  //================================================================================================
  // Done with everything, free our context and quit GLFW.

  planes.clear();
  glfwMakeContextCurrent(nullptr);
  glfwDestroyWindow(m_window);
  glfwTerminate();

  return 0;
}