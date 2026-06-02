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
#include <NCollection_Vec2.hxx>
#include <TCollection_AsciiString.hxx>

struct GLFWwindow;

//! GLFWwindow wrapper implementing Aspect_Window interface.
class Occt_glfw_win : public Aspect_Window
{
  DEFINE_STANDARD_RTTI_INLINE(Occt_glfw_win, Aspect_Window)
public:
  //! Main constructor.
  Occt_glfw_win(int theWidth, int theHeight, const TCollection_AsciiString& theTitle);
  Occt_glfw_win(GLFWwindow* GlfwWindow);

  //! Close the window.
  virtual ~Occt_glfw_win() { Close(); }

  //! Close the window.
  void Close();

  //! Return X Display connection.
  const Handle(Aspect_DisplayConnection) & GetDisplay() const { return myDisplay; }

  //! Return GLFW window.
  GLFWwindow* getGlfwWindow() { return myGlfwWindow; }

#ifndef __EMSCRIPTEN__
  //! Return native OpenGL context.
  Aspect_RenderingContext NativeGlContext() const;
#endif

  //! Return cursor position.
  NCollection_Vec2<int> CursorPosition() const;

public:
#ifndef __EMSCRIPTEN__
  //! Returns native Window handle
  virtual Aspect_Drawable NativeHandle() const override;
#endif

  //! Returns parent of native Window handle.
  virtual Aspect_Drawable NativeParentHandle() const override { return 0; }

  //! Applies the resizing to the window <me>
  virtual Aspect_TypeOfResize DoResize() override;

  //! Returns True if the window <me> is opened and False if the window is closed.
  virtual bool IsMapped() const override;

  //! Apply the mapping change to the window <me> and returns TRUE if the window is mapped at screen.
  virtual bool DoMapping() const override { return true; }

  //! Opens the window <me>.
  virtual void Map() const override;

  //! Closes the window <me>.
  virtual void Unmap() const override;

  virtual void Position(int& theX1, int& theY1, int& theX2,
                        int& theY2) const override
  {
    theX1 = myXLeft;
    theX2 = myXRight;
    theY1 = myYTop;
    theY2 = myYBottom;
  }

  //! Returns The Window RATIO equal to the physical WIDTH/HEIGHT dimensions.
  virtual double Ratio() const override
  {
    return double(myXRight - myXLeft) / double(myYBottom - myYTop);
  }

  //! Return window size.
  virtual void Size(int& theWidth, int& theHeight) const override
  {
    theWidth  = myXRight - myXLeft;
    theHeight = myYBottom - myYTop;
  }

  virtual Aspect_FBConfig NativeFBConfig() const override { return NULL; }

protected:
  Handle(Aspect_DisplayConnection) myDisplay;
  GLFWwindow*      myGlfwWindow;
  int myXLeft;
  int myYTop;
  int myXRight;
  int myYBottom;
};

#endif // _Occt_glfw_win_Header
