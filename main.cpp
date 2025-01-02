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

class MeshCube {
public:
  MeshCube(GLfloat scale, size_t numTriangles = 6 * 2 * 15 * 15) {
    // Figure out how many quads we have per edge.  There
    // is a minimum of 1.
    size_t numQuads = numTriangles / 2;
    size_t numQuadsPerFace = numQuads / 6;
    size_t numQuadsPerEdge = static_cast<size_t> (sqrt(numQuadsPerFace));
    if (numQuadsPerEdge < 1) { numQuadsPerEdge = 1; }

    // Construct a white square with the specified number of
    // quads as the +Z face of the cube.  We'll copy this and
    // then multiply by the correct face color, and we'll
    // adjust the coordinates by rotation to match each face.
    std::vector<GLfloat> whiteBufferData;
    std::vector<GLfloat> faceBufferData;
    for (size_t i = 0; i < numQuadsPerEdge; i++) {
      for (size_t j = 0; j < numQuadsPerEdge; j++) {

        // Modulate the color of each quad by a random luminance,
        // leaving all vertices the same color.
        GLfloat color = 0.5f + rand() * 0.5f / RAND_MAX;
        const size_t numTris = 2;
        const size_t numColors = 3;
        const size_t numVerts = 3;
        for (size_t c = 0; c < numColors * numTris * numVerts; c++) {
          whiteBufferData.push_back(color);
        }

        // Send the two triangles that make up this quad, where the
        // quad covers the appropriate fraction of the face from
        // -scale to scale in X and Y.
        GLfloat Z = scale;
        GLfloat minX = -scale + i * (2 * scale) / numQuadsPerEdge;
        GLfloat maxX = -scale + (i + 1) * (2 * scale) / numQuadsPerEdge;
        GLfloat minY = -scale + j * (2 * scale) / numQuadsPerEdge;
        GLfloat maxY = -scale + (j + 1) * (2 * scale) / numQuadsPerEdge;
        faceBufferData.push_back(minX);
        faceBufferData.push_back(maxY);
        faceBufferData.push_back(Z);

        faceBufferData.push_back(minX);
        faceBufferData.push_back(minY);
        faceBufferData.push_back(Z);

        faceBufferData.push_back(maxX);
        faceBufferData.push_back(minY);
        faceBufferData.push_back(Z);

        faceBufferData.push_back(maxX);
        faceBufferData.push_back(maxY);
        faceBufferData.push_back(Z);

        faceBufferData.push_back(minX);
        faceBufferData.push_back(maxY);
        faceBufferData.push_back(Z);

        faceBufferData.push_back(maxX);
        faceBufferData.push_back(minY);
        faceBufferData.push_back(Z);
      }
    }

    // Make a copy of the vertices for each face, then modulate
    // the color by the face color and rotate the coordinates to
    // put them on the correct cube face.

    // +Z is blue and is in the same location as the original
    // faces.
    {
      std::array<GLfloat, 3> modColor = { 0.0, 0.0, 1.0 };
      std::vector<GLfloat> myBufferData =
        colorModulate(whiteBufferData, modColor);

      // X = X, Y = Y, Z = Z
      std::array<GLfloat, 3> scales = { 1.0f, 1.0f, 1.0f };
      std::array<size_t, 3> indices = { 0, 1, 2 };
      std::vector<GLfloat> myFaceBufferData =
        vertexRotate(faceBufferData, indices, scales);

      // Catenate the colors onto the end of the
      // color buffer.
      colorBufferData.insert(colorBufferData.end(),
        myBufferData.begin(), myBufferData.end());

      // Catenate the vertices onto the end of the
      // vertex buffer.
      vertexBufferData.insert(vertexBufferData.end(),
        myFaceBufferData.begin(), myFaceBufferData.end());
    }

    // -Z is cyan and is in the opposite size from the
    // original face (mirror all 3).
    {
      std::array<GLfloat, 3> modColor = { 0.0, 1.0, 1.0 };
      std::vector<GLfloat> myBufferData =
        colorModulate(whiteBufferData, modColor);

      // X = -X, Y = -Y, Z = -Z
      std::array<GLfloat, 3> scales = { -1.0f, -1.0f, -1.0f };
      std::array<size_t, 3> indices = { 0, 1, 2 };
      std::vector<GLfloat> myFaceBufferData =
        vertexRotate(faceBufferData, indices, scales);

      // Catenate the colors onto the end of the
      // color buffer.
      colorBufferData.insert(colorBufferData.end(),
        myBufferData.begin(), myBufferData.end());

      // Catenate the vertices onto the end of the
      // vertex buffer.
      vertexBufferData.insert(vertexBufferData.end(),
        myFaceBufferData.begin(), myFaceBufferData.end());
    }

    // +X is red and is rotated -90 degrees from the original
    // around Y.
    {
      std::array<GLfloat, 3> modColor = { 1.0, 0.0, 0.0 };
      std::vector<GLfloat> myBufferData =
        colorModulate(whiteBufferData, modColor);

      // X = Z, Y = Y, Z = -X
      std::array<GLfloat, 3> scales = { 1.0f, 1.0f, -1.0f };
      std::array<size_t, 3> indices = { 2, 1, 0 };
      std::vector<GLfloat> myFaceBufferData =
        vertexRotate(faceBufferData, indices, scales);

      // Catenate the colors onto the end of the
      // color buffer.
      colorBufferData.insert(colorBufferData.end(),
        myBufferData.begin(), myBufferData.end());

      // Catenate the vertices onto the end of the
      // vertex buffer.
      vertexBufferData.insert(vertexBufferData.end(),
        myFaceBufferData.begin(), myFaceBufferData.end());
    }

    // -X is magenta and is rotated 90 degrees from the original
    // around Y.
    {
      std::array<GLfloat, 3> modColor = { 1.0, 0.0, 1.0 };
      std::vector<GLfloat> myBufferData =
        colorModulate(whiteBufferData, modColor);

      // X = -Z, Y = Y, Z = X
      std::array<GLfloat, 3> scales = { -1.0f, 1.0f, 1.0f };
      std::array<size_t, 3> indices = { 2, 1, 0 };
      std::vector<GLfloat> myFaceBufferData =
        vertexRotate(faceBufferData, indices, scales);

      // Catenate the colors onto the end of the
      // color buffer.
      colorBufferData.insert(colorBufferData.end(),
        myBufferData.begin(), myBufferData.end());

      // Catenate the vertices onto the end of the
      // vertex buffer.
      vertexBufferData.insert(vertexBufferData.end(),
        myFaceBufferData.begin(), myFaceBufferData.end());
    }

    // +Y is green and is rotated -90 degrees from the original
    // around X.
    {
      std::array<GLfloat, 3> modColor = { 0.0, 1.0, 0.0 };
      std::vector<GLfloat> myBufferData =
        colorModulate(whiteBufferData, modColor);

      // X = X, Y = Z, Z = -Y
      std::array<GLfloat, 3> scales = { 1.0f, 1.0f, -1.0f };
      std::array<size_t, 3> indices = { 0, 2, 1 };
      std::vector<GLfloat> myFaceBufferData =
        vertexRotate(faceBufferData, indices, scales);

      // Catenate the colors onto the end of the
      // color buffer.
      colorBufferData.insert(colorBufferData.end(),
        myBufferData.begin(), myBufferData.end());

      // Catenate the vertices onto the end of the
      // vertex buffer.
      vertexBufferData.insert(vertexBufferData.end(),
        myFaceBufferData.begin(), myFaceBufferData.end());
    }

    // -Y is yellow and is rotated 90 degrees from the original
    // around X.
    {
      std::array<GLfloat, 3> modColor = { 1.0, 1.0, 0.0 };
      std::vector<GLfloat> myBufferData =
        colorModulate(whiteBufferData, modColor);

      // X = X, Y = -Z, Z = Y
      std::array<GLfloat, 3> scales = { 1.0f, -1.0f, 1.0f };
      std::array<size_t, 3> indices = { 0, 2, 1 };
      std::vector<GLfloat> myFaceBufferData =
        vertexRotate(faceBufferData, indices, scales);

      // Catenate the colors onto the end of the
      // color buffer.
      colorBufferData.insert(colorBufferData.end(),
        myBufferData.begin(), myBufferData.end());

      // Catenate the vertices onto the end of the
      // vertex buffer.
      vertexBufferData.insert(vertexBufferData.end(),
        myFaceBufferData.begin(), myFaceBufferData.end());
    }
  }

  ~MeshCube() {
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
  MeshCube(const MeshCube&) = delete;
  MeshCube& operator=(const MeshCube&) = delete;
  bool initialized = false;
  GLuint colorBuffer = 0;
  GLuint vertexBuffer = 0;
  std::vector<GLfloat> colorBufferData;
  std::vector<GLfloat> vertexBufferData;

  // Multiply each triple of colors by the specified color.
  std::vector<GLfloat> colorModulate(std::vector<GLfloat> const& inVec,
    std::array<GLfloat, 3> const& clr) {
    std::vector<GLfloat> out;
    size_t elements = inVec.size() / 3;
    if (elements * 3 != inVec.size()) {
      // We don't have an even multiple of 3 elements, so bail.
      return out;
    }
    out = inVec;
    for (size_t i = 0; i < elements; i++) {
      for (size_t c = 0; c < 3; c++) {
        out[3 * i + c] *= clr[c];
      }
    }
    return out;
  }

  // Swizzle each triple of coordinates by the specified
  // index and then multiply by the specified scale.  This
  // lets us implement a poor-man's rotation matrix, where
  // we pick which element (0-2) and which polarity (-1 or
  // 1) to use.
  std::vector<GLfloat> vertexRotate(
    std::vector<GLfloat> const& inVec,
    std::array<size_t, 3> const& indices,
    std::array<GLfloat, 3> const& scales) {
    std::vector<GLfloat> out;
    size_t elements = inVec.size() / 3;
    if (elements * 3 != inVec.size()) {
      // We don't have an even multiple of 3 elements, so bail.
      return out;
    }
    out.resize(inVec.size());
    for (size_t i = 0; i < elements; i++) {
      for (size_t p = 0; p < 3; p++) {
        out[3 * i + p] = inVec[3 * i + indices[p]] * scales[p];
      }
    }
    return out;
  }
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
  // Make our geometry object, which will draw itself.
  /// @todo Replace this with a plane whose color we set.
  float radius = 0.25f;
  size_t quadsPerEdge = 10;
  size_t trianglesPerSide = 2 * quadsPerEdge * quadsPerEdge;
  // 6 faces
  size_t numTriangles = static_cast<size_t>(trianglesPerSide * 6);
  std::shared_ptr<MeshCube> roomCube = std::shared_ptr<MeshCube>(new MeshCube(radius, numTriangles));

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
    roomCube->draw();

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

  glfwMakeContextCurrent(nullptr);
  glfwDestroyWindow(m_window);
  glfwTerminate();

  return 0;
}