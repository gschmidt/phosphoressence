// __BEGIN_LICENSE__
// Copyright (C) 2009 Michael J. Broxton
// All Rights Reserved.
// __END_LICENSE__

/// \file GraphicsEngine.h
///
#ifndef __GRAPHICS_ENGINE_H__
#define __GRAPHICS_ENGINE_H__

// Qt
#include <QGLWidget>
#include <QGLFormat>
#include <QPoint>

// Vision Workbench
#include <vw/Core/Log.h>
#include <vw/Image/ImageResource.h>
#include <vw/Image/ImageViewRef.h>
#include <vw/Image/ImageView.h>
#include <vw/Image/Manipulation.h>
#include <vw/Image/Statistics.h>
#include <vw/FileIO/DiskImageResource.h>
#include <vw/FileIO/DiskImageView.h>
#include <vw/Math/BBox.h>
#include <vw/Math/Vector.h>
#include <vw/Math/Matrix.h>

// STL
#include <string>
#include <list>

#include <ScriptEngine.h>

// Forward declarations
namespace vw { 
namespace GPU {
  class GpuProgram; 
}}

class QMouseEvent;
class QWheelEvent;
class QPoint;

#define HORIZ_MESH_SIZE 64
#define VERT_MESH_SIZE 64
#define FRAMEBUFFER_SIZE 2048

/// Drawable
///
/// This is the base class for a drawable object.  A class that
/// inherits from Drawable is expected to implement the draw() method
/// to render something in OpenGL.  The subclass should assume that
/// the OpenGL viewport and projection matrices are properly
/// initialized, so all it needs to do is initialize the ModelView
/// matrix and draw using the OpenGL primatives.
///
struct Drawable {
  virtual void draw(float time, float gain) = 0;
};


// CUDA
void copy_image (int pbo_in, int width, int height);
void pboRegister(int pbo);
void pboUnregister(int pbo);

/// GraphicsEngine
///
/// The GraphicsEngine creates an OpenGL context, performs setup, and
/// drives rendering into the OpenGL framebuffer.  It also manages the
/// feedback texture -- which is central to PhosphorEssence's cool
/// effects!
///
class GraphicsEngine : public QGLWidget {
  Q_OBJECT
  
public:
  
  // Constructors/Destructor
  GraphicsEngine(QWidget *parent, QGLFormat const& frmt) : 
    QGLWidget(frmt, parent) {

     if (!format().sampleBuffers())
       std::cout << "\n\nCould not activate FSAA; results will be suboptimal\n\n";
     if (!format().doubleBuffer())
       std::cout << "\n\nCould not set double buffering; results will be suboptimal\n\n";

     //     std::cout << "\n***\nBuffers: " << format().sampleBuffers() << "\n***\n";

    setup();
  }
  virtual ~GraphicsEngine();
  
  // Set a default size for this widget.  This is usually overridden
  // by parent views.
  virtual QSize sizeHint () const { return QSize(500,500); }
  
  // Image Manipulation Methods
  void zoom(float scale);
  void size_to_fit();
  
  // Managing drawables
  void register_drawable(boost::shared_ptr<Drawable> drawable) {
    m_drawables.push_back(drawable);
  }

  void clear_drawables() {
    m_drawables.clear();
  }

signals:

  void pe_quit();

public slots:

  // This timer is used by the Texture Fetch thread to inform the
  // GraphicsEngine when new textures are available for drawing.
  // Timer callback is called 30 times per second.
  void timer_callback();

protected:

  // Setup
  void setup();
  void initializeGL();
  void resizeGL(int width, int height);
  void setup_mesh();

  // Event handlers
  void paintEvent(QPaintEvent *event);
  void mousePressEvent(QMouseEvent *event);
  void mouseMoveEvent(QMouseEvent *event);
  void mouseDoubleClickEvent(QMouseEvent *event);
  void wheelEvent(QWheelEvent *event);
  void enterEvent(QEvent *event);
  void leaveEvent(QEvent *event);
  void keyPressEvent(QKeyEvent *event);
  
private:

  // Drawing is driven by QPaintEvents, which call out to drawImage()
  // and drawLegend()
  void drawImage();
  void drawLegend(QPainter *painter);
  void updateCurrentMousePosition();
  void saveFeedback();

  // Graphic Elements (see GraphicElements.cc for imlementations)
  void drawFeedback();
  void drawDarkenCenter();
  void drawVectorField();
  void draw_milkdrop_waves();
  
  // Image & OpenGL
  GLuint m_feedback_texture;
  GLuint m_feedback_pbo;
  GLuint m_framebuffer_texture0;
  GLuint m_framebuffer_texture1;
  GLuint m_framebuffer;
  boost::shared_ptr<vw::GPU::GpuProgram> m_gpu_frontbuffer_program;
  boost::shared_ptr<vw::GPU::GpuProgram> m_gpu_backbuffer_program;

  // Drawables & Script Engines
  std::list<boost::shared_ptr<Drawable> > m_drawables;
  
  // Timers and updates
  QTimer *m_timer;
  double m_fps_last_time;
  double m_fps_avg;

  // Mouse positions and legend information
  QPoint lastPos;
  QPoint currentImagePos;
  std::string m_legend_status;

  // Dimensions & stats
  int m_viewport_width;
  int m_viewport_height;
  int m_framebuffer_width;
  int m_framebuffer_height;
  float m_aspect;
  float m_framebuffer_radius;

  // Feedback
  vw::Matrix<vw::Vector2> m_feedback_texcoords;
  vw::Matrix<vw::Vector2> m_feedback_screencoords;
  vw::Matrix<vw::Vector2> m_warped_screencoords;
  
  // Image Parameters
  vw::BBox2 m_current_viewport;
};

#endif  // __GRAPHICS_ENGINE_H__
