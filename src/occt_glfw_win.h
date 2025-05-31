// Copyright (c) 2019 OPEN CASCADE SAS
//
// This file is part of the examples of the Open CASCADE Technology software library.
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE

#ifndef _Occt_glfw_win_Header
#define _Occt_glfw_win_Header

#include <Aspect_DisplayConnection.hxx>
#include <Aspect_RenderingContext.hxx>
#include <Aspect_Window.hxx>
#include <Graphic3d_Vec.hxx>
#include <TCollection_AsciiString.hxx>

struct GLFWwindow;

//! GLFWwindow wrapper implementing Aspect_Window interface.
class Occt_glfw_win : public Aspect_Window
{
  DEFINE_STANDARD_RTTI_INLINE(Occt_glfw_win, Aspect_Window)
public:
  //! Main constructor.
  Occt_glfw_win (int theWidth, int theHeight, const TCollection_AsciiString& theTitle);
  Occt_glfw_win(GLFWwindow*      GlfwWindow);

  //! Close the window.
  virtual ~Occt_glfw_win() { Close(); }

  //! Close the window.
  void Close();

  //! Return X Display connection.
  const Handle(Aspect_DisplayConnection)& GetDisplay() const { return myDisplay; }

  //! Return GLFW window.
  GLFWwindow* getGlfwWindow() { return myGlfwWindow; }

#ifndef __EMSCRIPTEN__
  //! Return native OpenGL context.
  Aspect_RenderingContext NativeGlContext() const;
#endif

  //! Return cursor position.
  Graphic3d_Vec2i CursorPosition() const;

public:

#ifndef __EMSCRIPTEN__
  //! Returns native Window handle
  virtual Aspect_Drawable NativeHandle() const Standard_OVERRIDE;
#endif

  //! Returns parent of native Window handle.
  virtual Aspect_Drawable NativeParentHandle() const Standard_OVERRIDE { return 0; }

  //! Applies the resizing to the window <me>
  virtual Aspect_TypeOfResize DoResize() Standard_OVERRIDE;

  //! Returns True if the window <me> is opened and False if the window is closed.
  virtual Standard_Boolean IsMapped() const Standard_OVERRIDE;

  //! Apply the mapping change to the window <me> and returns TRUE if the window is mapped at screen.
  virtual Standard_Boolean DoMapping() const Standard_OVERRIDE { return Standard_True; }

  //! Opens the window <me>.
  virtual void Map() const Standard_OVERRIDE;

  //! Closes the window <me>.
  virtual void Unmap() const Standard_OVERRIDE;

  virtual void Position (Standard_Integer& theX1, Standard_Integer& theY1,
                         Standard_Integer& theX2, Standard_Integer& theY2) const Standard_OVERRIDE
  {
    theX1 = myXLeft;
    theX2 = myXRight;
    theY1 = myYTop;
    theY2 = myYBottom;
  }

  //! Returns The Window RATIO equal to the physical WIDTH/HEIGHT dimensions.
  virtual Standard_Real Ratio() const Standard_OVERRIDE
  {
    return Standard_Real (myXRight - myXLeft) / Standard_Real (myYBottom - myYTop);
  }

  //! Return window size.
  virtual void Size (Standard_Integer& theWidth, Standard_Integer& theHeight) const Standard_OVERRIDE
  {
    theWidth  = myXRight - myXLeft;
    theHeight = myYBottom - myYTop;
  }

  virtual Aspect_FBConfig NativeFBConfig() const Standard_OVERRIDE { return NULL; }

protected:
  Handle(Aspect_DisplayConnection) myDisplay;
  GLFWwindow*      myGlfwWindow;
  Standard_Integer myXLeft;
  Standard_Integer myYTop;
  Standard_Integer myXRight;
  Standard_Integer myYBottom;
};

#endif // _Occt_glfw_win_Header
