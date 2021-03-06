// __BEGIN_LICENSE__
// Copyright (C) 2009 Michael J. Broxton
// All Rights Reserved.
// __END_LICENSE__

/// \file GraphicsEngine.cc
///

#ifdef __linux__
// This is required to get prototypes, according to the opengl linux abi
#define GL_GLEXT_PROTOTYPES 1
#endif

#ifdef __APPLE__
#include <OpenGL/gl.h>
#include <OpenGL/glu.h>
#include <AGL/agl.h>
#else // Linux
#include <GL/gl.h>
#include <GL/glu.h>
#include <GL/glext.h>
#endif 

#include <QtGui>
//#include <cuda.h>

// Vision Workbench
#include <vw/Image.h>
#include <vw/FileIO.h>
#include <vw/Math.h>
using namespace vw;

#include <GraphicsEngine.h>
#include <GpuProgram.h>
#include <PeParameters.h>
using namespace vw::GPU;

#include <fstream>

#include <cuda.h>
#include <cudaGL.h>
#include <cuda_runtime_api.h>
#include <cuda_gl_interop.h>


// Switch from uin8 to floating point textures
//#define PE_GL_FORMAT GL_RGBA16F_ARB
#define PE_GL_FORMAT GL_RGBA

// --------------------------------------------------------------
//                       GLSL DEBUGGING
// --------------------------------------------------------------

void print_shader_info_log(GLuint obj)
{
  int infologLength = 0;
  int charsWritten  = 0;
  char *infoLog;

  glGetShaderiv(obj, GL_INFO_LOG_LENGTH,&infologLength);

  if (infologLength > 1) {
    infoLog = (char *)malloc(infologLength);
    glGetShaderInfoLog(obj, infologLength, &charsWritten, infoLog);
    std::ostringstream err;
    err << "<h4>An error occured while compiling the GLSL shader:</h4><p><h5><tt>" << infoLog << "</tt></h5>";
    QMessageBox::critical(0, "GLSL Shader Error", 
                            err.str().c_str());
    free(infoLog);
  }
}

void print_program_info_log(GLuint obj)
{
  int infologLength = 0;
  int charsWritten  = 0;
  char *infoLog;
  
  glGetProgramiv(obj, GL_INFO_LOG_LENGTH,&infologLength);
  
  if (infologLength > 1) {
    infoLog = (char *)malloc(infologLength);
    glGetProgramInfoLog(obj, infologLength, &charsWritten, infoLog);
    std::ostringstream err;
    err << "<h4>An error occured while linking the GLSL program:</h4><p><h5><tt>" << infoLog << "</tt></h5>";
    QMessageBox::critical(0, "GLSL Program Error", 
                          err.str().c_str());
    printf("%s\n",infoLog);
    free(infoLog);
  }
}

void check_gl_errors( void ) 
{ 
  GLenum errCode; 
  const GLubyte *errStr; 
  if ( ( errCode = glGetError( ) ) != GL_NO_ERROR ) { 
    errStr = gluErrorString( errCode ); 
    std::cout << "OpenGL ERROR (" << int(errCode) << "): " << errStr << "\n";
  } 
} 
// --------------------------------------------------------------
//               GraphicsEngine Public Methods
// --------------------------------------------------------------

GraphicsEngine::~GraphicsEngine() {

  // De-allocate feedback texture and PBO
  glDeleteTextures(1, &m_feedback_texture);
  //  glDeleteBuffers(1, &m_feedback_pbo);

  // De-allocate any previously allocated textures or framebuffers
  glDeleteTextures(1, &m_framebuffer_texture0);
  glDeleteTextures(1, &m_framebuffer_texture1);
  glDeleteFramebuffersEXT(1, &m_framebuffer);
}

// --------------------------------------------------------------
//             GraphicsEngine Private Methods
// --------------------------------------------------------------
void GraphicsEngine::timer_callback() {
  update();
}


void GraphicsEngine::drawImage() {
  if (pe_parameters().get_value("exit")) 
    emit pe_quit();

  // ------------------------ <Scripts> -------------------------

  // Call out to any PhosphorScripts that are running on the
  // JavaScript VM, allowing them to update parameters if they would
  // like.
  pe_script_engine().set_parameter("aspect", m_aspect);
  pe_script_engine().execute("pe_animate()");

  // Make this context current, and store the current OpenGL state
  // before we start to modify it.
  makeCurrent();
  
  // ------------------------ <FrameBuffer> -------------------------
 
  // Activate the framebuffer.  All of the following drawing is done
  // in the framebuffer so that we have a larger area available to
  // draw in.
  glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, m_framebuffer);
  glDrawBuffer(GL_COLOR_ATTACHMENT0_EXT);
  glReadBuffer(GL_COLOR_ATTACHMENT0_EXT);

  // Set the background color and viewport.
  qglClearColor(QColor(0, 0, 0)); // Black Background
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  glViewport(0,0,m_framebuffer_width,m_framebuffer_height);

  // Set up the orthographic view of the scene.  The exact extent of
  // the view onto the scene depends on the current panning and zoom
  // in the UI.
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();

  glOrtho(-m_framebuffer_radius, m_framebuffer_radius, 
          -m_framebuffer_radius, m_framebuffer_radius, 
          -1.0, 1.0);

  // Set up the modelview matrix, and bind the image as the texture we
  // are about to use.
  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();

  // Start by drawing a black frame.
  glColor4f(0.0,0.0,0.0,1.0);
  glBegin(GL_QUADS);
  glVertex2f( -m_framebuffer_radius, -m_framebuffer_radius);
  glVertex2f( m_framebuffer_radius, -m_framebuffer_radius);
  glVertex2f( m_framebuffer_radius, m_framebuffer_radius);
  glVertex2f( -m_framebuffer_radius, m_framebuffer_radius);
  glEnd() ;

  // -----------------------
  // FEEDBACK TEXTURE 
  // ----------------------
  drawFeedback();

  // -----------------------
  // Darken Center
  // -----------------------
  drawDarkenCenter();

  // -----------------------
  // Motion Vectors
  // ----------------------
  drawVectorField();

  // Call the python environment and allow it to render whatever it wants using PyOpenGL
  pe_script_engine().execute("pe_render()");


  // Run through the list of drawables, giving them each a chance to
  // render into the display.
  if (pe_script_engine().get_parameter("wave_enabled") ) {
    std::list<boost::shared_ptr<Drawable> >::iterator iter = m_drawables.begin();
    for (int i = 0; iter != m_drawables.end(); ++i, ++iter) {
      if ( i == pe_script_engine().get_parameter("wave_mode") ) {
        (*iter)->draw(pe_time(),
                      pe_script_engine().get_parameter("decay"));
         break;
      }
    }
  }

  // ----------------- <Feedback> ----------------------

  // Save the feedback texture 
  saveFeedback();

  // -------------- <Render to Screen> -----------------

  // Make the real screen the OpenGL target
  glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);

  qglClearColor(QColor(0, 0, 0)); // Black Background
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  glViewport(0,0,m_viewport_width,m_viewport_height);
  
  // Set up the orthographic view of the scene.  The exact extent of
  // the view onto the scene depends on the current panning and zoom
  // in the UI.
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  glOrtho(-m_aspect, m_aspect, -1.0, 1.0, -1.0, 1.0);

  // Set up the modelview matrix, and bind the image as the texture we
  // are about to use.
  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();

  // Draw the framebuffer to the real screen.
  glEnable( GL_TEXTURE_2D );
  glBindTexture( GL_TEXTURE_2D, m_framebuffer_texture0 );

  m_gpu_frontbuffer_program->install();
  m_gpu_frontbuffer_program->set_input_int("backbuffer_texture", 0);
  m_gpu_frontbuffer_program->set_input_float("framebuffer_radius", float(m_framebuffer_width));
  m_gpu_frontbuffer_program->set_input_float("time", pe_time());
  m_gpu_frontbuffer_program->set_input_float("gamma", pe_script_engine().get_parameter("gamma"));
  m_gpu_frontbuffer_program->set_input_float("invert", pe_script_engine().get_parameter("invert"));
  m_gpu_frontbuffer_program->set_input_float("brighten", pe_script_engine().get_parameter("brighten"));
  m_gpu_frontbuffer_program->set_input_float("darken", pe_script_engine().get_parameter("darken"));
  m_gpu_frontbuffer_program->set_input_float("solarize", pe_script_engine().get_parameter("solarize"));
        
  // Determine the dimensions of the sub-window for drawing from the
  // framebuffer texture.
  float w_texture = 0.5 * m_aspect/m_framebuffer_radius; // r_texture * ( w_object / r_object )
  float h_texture = 0.5 * 1.0/m_framebuffer_radius;      // r_texture * ( h_object / r_object )

  //  We will draw the image as a texture on this quad.
  qglColor(Qt::blue);
  glBegin(GL_QUADS);
  glTexCoord2f( 0.5-w_texture, 0.5-h_texture );
  glVertex2d( -m_aspect, -1.0);
  glTexCoord2f( 0.5+w_texture, 0.5-h_texture ); 
  glVertex2d( m_aspect, -1.0);
  glTexCoord2f( 0.5+w_texture, 0.5+h_texture );
  glVertex2d( m_aspect, 1.0);
  glTexCoord2f( 0.5-w_texture, 0.5+h_texture );
  glVertex2d( -m_aspect, 1.0);
  glEnd() ;

  glBindTexture( GL_TEXTURE_2D, 0 );
  glDisable( GL_TEXTURE_2D );
  m_gpu_frontbuffer_program->uninstall();

  if (pe_script_engine().get_parameter("show_fps") != 0) {
    char fps_cstr[255];
    sprintf(fps_cstr, "FPS: %0.2f", m_fps_avg);
    QString fps_str(fps_cstr);
    this->renderText(20,20,fps_str);
  }

  // Swap the buffer and render to the screen.
  //  this->swapBuffers();

  // Recompute FPS
  double new_time = double(vw::Stopwatch::microtime()) / 1.0e6;
  float fps = 1.0/(new_time - m_fps_last_time);
  m_fps_avg = 0.01 * fps + 0.99 * m_fps_avg;
  pe_script_engine().set_parameter("fps", fps);
  // For debugging:
  //  std::cout << "FPS: " << fps << "\n";

  m_fps_last_time = new_time;
  pe_script_engine().set_parameter("frame", pe_script_engine().get_parameter("frame") + 1.0);
}

void GraphicsEngine::drawLegend(QPainter* painter) {
  const int Margin = 11;
  const int Padding = 6;
  QTextDocument textDocument;
  textDocument.setDefaultStyleSheet("* { color: #00FF00; font-family: courier, serif; font-size: 12 }");
  std::ostringstream legend_text;
  legend_text << "<p align=\"right\">TESTING" << m_legend_status << "<br>" << "</p>";
  textDocument.setHtml(legend_text.str().c_str());
  textDocument.setTextWidth(textDocument.size().width());
  
  QRect rect(QPoint(0,0), textDocument.size().toSize() + QSize(2 * Padding, 2 * Padding));
  painter->translate(10,10);//width() - rect.width() - Margin, height() - rect.height() - Margin);
  //     painter->setPen(QColor(255, 239, 239));
  //     painter->drawRect(rect);
  painter->translate(Padding, Padding);
  textDocument.drawContents(painter);
}

void GraphicsEngine::updateCurrentMousePosition() {
  float x_loc = m_current_viewport.min().x() + m_current_viewport.width() * float(lastPos.x()) / m_viewport_width;
  float y_loc = m_current_viewport.min().y() + m_current_viewport.height() * float(lastPos.y()) / m_viewport_height;
  currentImagePos = QPoint(x_loc,y_loc);
}


// --------------------------------------------------------------
//             GraphicsEngine Setup Methods
// --------------------------------------------------------------
void GraphicsEngine::setup() {
  if (!QGLFormat::hasOpenGL()) {
    vw::vw_out(0) << "This system has no OpenGL support.\nExiting\n\n";
    exit(1);
  }

  // Feedback
  m_feedback_texcoords.set_size(HORIZ_MESH_SIZE + 1, VERT_MESH_SIZE + 1);
  m_feedback_screencoords.set_size(HORIZ_MESH_SIZE + 1, VERT_MESH_SIZE + 1);
  m_warped_screencoords.set_size(HORIZ_MESH_SIZE + 1, VERT_MESH_SIZE + 1);

  m_fps_avg = 30.0;

  // Set mouse tracking
  this->setMouseTracking(true);

  // Set the size policy that the widget can grow or shrink and still
  // be useful.
  this->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
}

void GraphicsEngine::saveFeedback() {

  // Activate the framebuffer.  All of the following steps pull data
  // from the framebuffer rather than the main screen.
  glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, m_framebuffer);
  // glDrawBuffer(GL_COLOR_ATTACHMENT0_EXT);
  // glReadBuffer(GL_COLOR_ATTACHMENT0_EXT);

  // -------------------------------------------
  // Old Code for saving the feedback texture...
  // -------------------------------------------
  glBindTexture(GL_TEXTURE_2D, m_feedback_texture);
  glCopyTexImage2D(GL_TEXTURE_2D, 0, PE_GL_FORMAT, 0, 0, m_framebuffer_width, m_framebuffer_height, 0);
  glBindTexture(GL_TEXTURE_2D, 0);

  // -- Save Feedback --
  // glBindBuffer(GL_PIXEL_PACK_BUFFER, m_feedback_pbo);
  // glReadPixels(0,0,m_framebuffer_width,m_framebuffer_height,GL_BGRA,GL_UNSIGNED_BYTE,NULL);
  // glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);

  // -- Debug --
  // vw::uint8* data = (vw::uint8*)glMapBuffer(GL_PIXEL_PACK_BUFFER, GL_READ_ONLY);
  // uint8 lo, hi;
  // ImageView<PixelRGBA<uint8> > view(m_framebuffer_width, m_framebuffer_height);
  //  int size = m_framebuffer_width * m_framebuffer_height * 4 * sizeof(vw::uint8);
  // memcpy(view.data(), data, size);
  // min_max_channel_values(view,lo,hi);
  // std::cout << "Writing a frame: " << int(lo) << " " << int(hi) << "\n";
  // write_image("test.tif", view);
  // glUnmapBuffer(GL_PIXEL_PACK_BUFFER);
  // Thread::sleep_ms(2000);
  // -- Debug --
  
  // -- CUDA --
  // pboRegister(m_feedback_pbo);
  // copy_image(m_feedback_pbo, m_framebuffer_width, m_framebuffer_height);

  // According to the CUDA API reference, you must unbind the buffer
  // object before writing to it.  You can rebind it immediately
  // afterwards.  (Todo: Test this??)
  //  pboUnregister(m_feedback_pbo);

  // glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);
  // glBindBuffer(GL_PIXEL_UNPACK_BUFFER, m_feedback_pbo);
  // glBindTexture(GL_TEXTURE_2D, m_feedback_texture);
  // glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 
  //                 m_framebuffer_width, m_framebuffer_height,
  //                 GL_BGRA, GL_UNSIGNED_BYTE, NULL);
  // glBindTexture(GL_TEXTURE_2D, 0);
  // glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
}

void GraphicsEngine::initializeGL() {  

  // Set up the GLSL fragment shader.
  std::string resources_dir = pe_resources_directory();
  m_gpu_frontbuffer_program = create_gpu_program(resources_dir + "/shaders/frontbuffer.glsl");
  m_gpu_backbuffer_program = create_gpu_program(resources_dir + "/shaders/backbuffer.glsl");
//                                                 std::vector<int>(),
//                                                 resources_dir + "/shaders/backbuffer_vertex.glsl",
//                                                 std::vector<int>());

  // Generate the feedback texture
  glGenTextures(1, &m_feedback_texture);
  glBindTexture(GL_TEXTURE_2D, m_feedback_texture);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT );
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT );
  // glTexParameterf(GL_TEXTURE_2D, GL_GENERATE_MIPMAP, GL_TRUE );
  glBindTexture(GL_TEXTURE_2D, 0);

  // Crank up the anisotropic filtering.  Not totally sure what this
  // does, but it might produce crisper images for filters that really
  // stretch the texture.
  GLfloat largest_supported_anisotropy;
  glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &largest_supported_anisotropy);

  // Create the framebuffer & framebuffer texture
  glGenFramebuffersEXT(1, &m_framebuffer);
  glGenTextures(1, &m_framebuffer_texture0);
  glGenTextures(1, &m_framebuffer_texture1);
  glBindTexture(GL_TEXTURE_2D, m_framebuffer_texture0);
  glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, largest_supported_anisotropy);
  glBindTexture(GL_TEXTURE_2D, m_framebuffer_texture1);
  glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, largest_supported_anisotropy);
  glBindTexture(GL_TEXTURE_2D, 0);

  // Uncomment to get rid of the tearing (i.e. tearing)!
#ifdef __APPLE__
  // AGLContext aglContext;
  // aglContext = aglGetCurrentContext();
  // GLint swapInt = 1;
  // aglSetInteger(aglContext, AGL_SWAP_INTERVAL, &swapInt);
  this->setAutoBufferSwap(false);
#endif

  // Enable hardware anti-aliasing
  glEnable(GL_MULTISAMPLE);
  
  // Set the grid size
  pe_script_engine().set_parameter("meshx", HORIZ_MESH_SIZE);
  pe_script_engine().set_parameter("meshy", VERT_MESH_SIZE);

  // Now that GL is setup, we can start the Qt Timer
  m_timer = new QTimer(this);
  connect(m_timer, SIGNAL(timeout()), this, SLOT(timer_callback()));
  m_timer->start(33.0); // Limit frame rate to ~30 fps
}

void GraphicsEngine::resizeGL(int width, int height) {
  makeCurrent();

  // Set the current viewport width/height
  m_viewport_width = width;
  m_viewport_height = height;

  m_aspect = float(m_viewport_width) / m_viewport_height;
  m_framebuffer_radius = sqrt(1+pow(m_aspect,2));

  // Compute framebuffer dimensions.  The framebuffer is a square that
  // circumscribes the circle that circumscribes the rectangle of the
  // viewport.  (Did you follow that?)  (Though, for now we override
  // this with a fixed size defined above.)
  //
  //  m_framebuffer_width = int( roundf(sqrtf(powf(m_viewport_width,2) + powf(m_viewport_height,2))) );
  m_framebuffer_width = FRAMEBUFFER_SIZE;
  m_framebuffer_height = m_framebuffer_width;  

  //------------------------------------
  // Set up the framebuffer and textures
  //------------------------------------

  // De-allocate any previously allocated textures or framebuffers
  // glDeleteBuffers(1, &m_feedback_pbo);

  int size = m_framebuffer_width * m_framebuffer_height * 4 * sizeof(vw::uint8);
  // std::cout << "Framebuffer Dimensions: " 
  //           << m_framebuffer_width << " " << m_framebuffer_height 
  //           << "   (" << size << " bytes)\n";

  // Allocate two pixel buffer object to store (and modify) the
  // feedback texture.  One of these is the input into the cuda
  // shaders, and one is the output.
  // cudaGLSetGLDevice ( 0 );
  // glGenBuffers(1, &m_feedback_pbo);
  // glBindBuffer(GL_ARRAY_BUFFER, m_feedback_pbo);
  // glBufferData(GL_ARRAY_BUFFER, size, NULL, GL_DYNAMIC_COPY);
  // glBindBuffer(GL_ARRAY_BUFFER, 0);
  // pboRegister(m_feedback_pbo);

  // Create the framebuffer texture (for rendering...)
  glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, m_framebuffer);
  glBindTexture(GL_TEXTURE_2D, m_framebuffer_texture0);
  glTexImage2D(GL_TEXTURE_2D, 0, PE_GL_FORMAT, 
               m_framebuffer_width, m_framebuffer_height, 
               0, GL_BGRA, GL_UNSIGNED_BYTE, NULL);
  glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT,
                            GL_TEXTURE_2D, m_framebuffer_texture0, 0);
  glBindTexture(GL_TEXTURE_2D, m_framebuffer_texture1);
  glTexImage2D(GL_TEXTURE_2D, 0, PE_GL_FORMAT, 
               m_framebuffer_width, m_framebuffer_height, 
               0, GL_BGRA, GL_UNSIGNED_BYTE, NULL);
  glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT1_EXT,
                            GL_TEXTURE_2D, m_framebuffer_texture1, 0);
  glDrawBuffer(GL_COLOR_ATTACHMENT0_EXT);
  glReadBuffer(GL_COLOR_ATTACHMENT0_EXT);
  GLenum status = glCheckFramebufferStatusEXT(GL_FRAMEBUFFER_EXT);
  if (status != GL_FRAMEBUFFER_COMPLETE_EXT)
    vw_throw(vw::LogicErr() << "GraphicsEngine::initializeGl() - could not initialize framebuffer.\n");
  glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);

  // Setup the feedback texture buffer
  glBindTexture(GL_TEXTURE_2D, m_feedback_texture);
  glTexImage2D(GL_TEXTURE_2D, 0, PE_GL_FORMAT, 
               m_framebuffer_width, m_framebuffer_height,
               0, GL_BGRA, GL_UNSIGNED_BYTE, NULL);
  glBindTexture(GL_TEXTURE_2D, 0);

  //------------------------------------
  // Set up the warp mesh
  //------------------------------------
  setup_mesh();
}

void GraphicsEngine::setup_mesh() {

  float aspect = float(m_viewport_width) / m_viewport_height;
  float framebuffer_radius = sqrt(1+pow(aspect,2));

  double texture_w = 1.0;
  double texture_h = 1.0;
  double screen_w = 2.0*framebuffer_radius;  
  double screen_h = 2.0*framebuffer_radius;
  
  for (int i = 0 ; i < HORIZ_MESH_SIZE + 1 ; i++) {
    for (int j = 0 ; j < VERT_MESH_SIZE + 1 ; j++) {
      m_feedback_texcoords(i,j)[0] = i * texture_w / HORIZ_MESH_SIZE;
      m_feedback_texcoords(i,j)[1] = j * texture_h / VERT_MESH_SIZE;

      m_feedback_screencoords(i,j)[0] = -framebuffer_radius + (i * screen_w) / HORIZ_MESH_SIZE;
      m_feedback_screencoords(i,j)[1] = -framebuffer_radius + (j * screen_h) / VERT_MESH_SIZE;
    }
  }
}



// --------------------------------------------------------------
//             GraphicsEngine Event Handlers
// --------------------------------------------------------------

void GraphicsEngine::paintEvent(QPaintEvent * /* event */) { 
  QPainter painter(this);
  drawImage();
  //  drawLegend(&painter);
}

void GraphicsEngine::mousePressEvent(QMouseEvent *event) { 
  grabKeyboard();
  lastPos = event->pos();
  updateCurrentMousePosition();
}

void GraphicsEngine::mouseMoveEvent(QMouseEvent *event) {
  // Left mouse button moves the image around
  if (event->buttons() & Qt::LeftButton) {
    float x_diff = float(event->x() - lastPos.x()) / m_viewport_width;
    float y_diff = float(event->y() - lastPos.y()) / m_viewport_height;
    float ticks;

    std::ostringstream s; 

    // do something here?
  }
  // Regardless, we store the current position for the text legend.
  lastPos = event->pos();
  updateCurrentMousePosition();
  //  update();
}

void GraphicsEngine::mouseDoubleClickEvent(QMouseEvent * /*event*/) {
  //  update();
}

void GraphicsEngine::wheelEvent(QWheelEvent *event) {
  // int num_degrees = event->delta() / 8;
  // float num_ticks = num_degrees / 15;

  // float scale = pow(2,num_ticks/5);

  // if (m_gain * scale >= 0.0 && m_gain * scale <= 1.0)
  //   m_gain *= scale;
  
  // //  zoom(scale);

  // m_show_legend = true;
  grabKeyboard();
  lastPos = event->pos();
  updateCurrentMousePosition();
}


void GraphicsEngine::enterEvent(QEvent */*event*/) {
  //  m_show_legend = true;
  grabKeyboard();
  //  update();
}

void GraphicsEngine::leaveEvent(QEvent */*event*/) {
  //  m_show_legend = false;
  releaseKeyboard();
  //  update();
}

void GraphicsEngine::keyPressEvent(QKeyEvent *event) {

  std::ostringstream s; 
  
  switch (event->key()) {
  case Qt::Key_Left: 
    pe_script_engine().execute("PePreset.prev_preset();");
    break;
  case Qt::Key_Right:  
    pe_script_engine().execute("PePreset.next_preset();");
    break;

  case Qt::Key_R:  
    pe_script_engine().execute("PePreset.random_preset();");
    break;

  case Qt::Key_Up:  
    pe_script_engine().set_parameter("ifs_mode", pe_script_engine().get_parameter("ifs_mode") + 1.0);
    break;
  case Qt::Key_Down:  
    pe_script_engine().set_parameter("ifs_mode", pe_script_engine().get_parameter("ifs_mode") - 1.0);
    break;
  case Qt::Key_I:  
    pe_script_engine().set_parameter("invert", 1.0-pe_script_engine().get_parameter("invert"));
    break;
  default: 
    QWidget::keyPressEvent(event);
  }
}

