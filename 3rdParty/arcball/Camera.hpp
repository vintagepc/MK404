//
//    Copyright 2011 Nicholas Pleschko <nicholas@nigi.li>
//
//    ============================================
//
//    This file is part of SimpleArcballCamera.
//
//    SimpleArcballCamera is free software: you can redistribute it and/or
//    modify it under the terms of the GNU General Public License as published
//    by the Free Software Foundation, either version 3 of the License, or
//    (at your option) any later version.
//
//    SimpleArcballCamera is distributed in the hope that it will be useful,
//    but WITHOUT ANY WARRANTY; without even the implied warranty of
//    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//    GNU General Public License for more details.
//
//    You should have received a copy of the GNU General Public License
//    along with SimpleArcballCamera.  If not,
//    see <http://www.gnu.org/licenses/>.

#ifndef SimpleArcballCamera_Camera_hpp
#define SimpleArcballCamera_Camera_hpp

#include "ArcBall.hpp"  // for ArcBall
#include "Matrix.hpp"   // for mat4
#include "Vectors.hpp"  // for vec3, vec2


//  Represents a camera. Enables the user to move the camera and at the end
//  get the according view transformation matrix.
class Camera {

private:
    // The arcball used for rotation with mouse movement
    gl::ArcBall arcBall_;

    // The current transformation matrix
    math::mat4 viewMatrix_;

	// The point in space where the camera looks at
    math::vec3 center_;

    // Vector describing the direction from the eye to the center.
    // I store this vector rather than the eye position because I want
    // to be able to move the camera with the center (needed for panning)
    math::vec3 eyeDir_;

    // A factor multiplied with zoom deltas to compensate for different
    // scroll wheel speeds.
    float zoomSpeed_ = 10.f;

    // The current width and height of the window. Needed for accurate behaivour
    // according to the mouse movements
    float windowWidth_ = 200.f;
    float windowHeight_ = 200.f;

    // The last and current mouse position (x,y) in window space. Needed for
    // getting the relative mouse movement for panning and rotation
    math::vec2 lastMousePos_;
    math::vec2 currentMousePos_;

    // If true the user currently is in panning or zooming mode
    enum Mode {IDLE, PANNING, ZOOMING};
    Mode currentMode_ = Mode::IDLE;


public:

    // ========================= CAMERA SETUP ================================

    // Creates a new camera with eye position (0,0,-1) looking at (0,0,0)
    // the up vector is always (0,1,0)
    Camera();

    // Creates a new camera with specified eye and center position
    // the up vector is always (0,1,0)
    Camera(float eyex, float eyey, float eyez, float centerx, float centery,
           float centerz);

    void setCenter(float x, float y, float z);
    void setEye(float x, float y, float z);

    void setCurrentMousePos(float x, float y);
    void setWindowSize(float w, float h);

    // ========================= CAMERA MOVEMENT =============================



    // Begins and ends the rotation state. Call this method when user clicks
    void beginRotate();
    void endRotate();

    // Begins and ends the panning state. Call this method when user clicks
    // for example the right button.
    void beginPan();
    void endPan();

    // Begins and ends the zooming state. Call this method when user clicks
    // for example the middle button.
    void beginZoom();
    void endZoom();

    // Directly moves the center by deltax in the x direction and deltay in
    // the y direction of the current viewing plane. Use this method for panning
    // with the keyboard. For panning with mouse interaction rather use the
    // combination: setCurrentMousePos() -> beginPan() -> setCurrentMousePos()
    //  -> endPan()
    void pan(float deltax, float deltay);

    // Moves the camera away from or towards the center. If you use some mouse
    // movement (not the scroll wheel) to scroll, rather use beginZoom() and
    // endZoom()
    void zoom(float deltaz);


    // ========================= MATRIX GETTER ===============================

    // Get the current camera matrix in the right format for opengl. Load this
    // matrix into the OpenGL context before drawing to get the right camera
    // view.
    const float * getViewMatrix();


private:

    void updatePanningPosition();

    // Updates the current view matrix
    void updateViewMatrix();

    // Gets the up and right vector from the current view matrix
    math::vec3 currentUp();
    math::vec3 currentRight();

    void initConstants();
};

#endif
