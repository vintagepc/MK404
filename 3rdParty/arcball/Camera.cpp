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

#include <iostream>

#include "Camera.hpp"

// ========================= CAMERA SETUP ================================ 

Camera::Camera() : center_(0,0,0), eyeDir_(0,0,-1)
{
    initConstants();
    updateViewMatrix();
}

Camera::Camera(float eyex, float eyey, float eyez, float centerx, float centery,
               float centerz)
    : center_(centerx,centery,centerz),
      eyeDir_(centerx-eyex,centery-eyey,centerz-eyez)
{
    initConstants();
    updateViewMatrix();
}

void Camera::initConstants()
{
    currentMode_ = IDLE;
    zoomSpeed_ = 10.0f;
    windowWidth_ = 200;
    windowHeight_ = 200;
}

// When explicitly setting the center the global eye position should stay the
// same. Therefore this method first computes the global eye position, then
// moves the center and in the end recomputes the relative eye direction.
void Camera::setCenter(float x, float y, float z)
{
    math::vec3 eye = center_ - eyeDir_;
    center_.set(x, y, z);
    eyeDir_.set(center_[0] - eye[0], center_[1] - eye[1], center_[2] - eye[2]);
    updateViewMatrix();
}

// Sets the eye position to x,y,z by computing the relative vector from x,y,z
// to the camera center.
void Camera::setEye(float x, float y, float z)
{
    eyeDir_.set(center_[0] - x, center_[1] - y, center_[2] - z);
    updateViewMatrix();
}

// Updates the current mouse position. Needs to invert the y coordinate for
// my computations but is ok for the arcball.
void Camera::setCurrentMousePos(float x, float y)
{
    arcBall_.set_cur(x, y);

    y = windowHeight_ - y;
    
    lastMousePos_ = currentMousePos_;
    currentMousePos_.set(x, y);
    
    if (currentMode_ != IDLE) {
        // Compute delta
        math::vec2 mouseDelta = currentMousePos_ - lastMousePos_;
        
        switch (currentMode_) {
            case PANNING:
                pan(mouseDelta[0], mouseDelta[1]);
                break;
                
            case ZOOMING:
                zoom(mouseDelta[1]);
            default:
                break;
        }
    }else{
        updateViewMatrix();
    }
}

// Updates the window size both in this class and in the arcball.
void Camera::setWindowSize(float w, float h)
{
    arcBall_.set_win_size(w, h);
    
    windowWidth_ = w;
    windowHeight_ = h;
}

// ========================= CAMERA MOVEMENT ============================= 


// Starts the arcballs rotation
void Camera::beginRotate()
{
    arcBall_.begin_drag();
}

// End the arcballs rotation
void Camera::endRotate()
{
    arcBall_.end_drag();
}

// Begins the panning mode
void Camera::beginPan()
{
    currentMode_ = PANNING;
}

// Ends the panning mode
void Camera::endPan()
{
    currentMode_ = IDLE;
}

// Begins the panning mode
void Camera::beginZoom()
{
    currentMode_ = ZOOMING;
}

// Ends the panning mode
void Camera::endZoom()
{
    currentMode_ = IDLE;
}

// Moves the center and eye in the viewplane directions
void Camera::pan(float x, float y)
{
    x = x / windowWidth_;
    y = y / windowHeight_;
    
    float length = 2 * eyeDir_.length() * tan(30*M_PI/180.0f);
    
    math::vec3 deltax = currentRight() * - x * (length * (windowWidth_/windowHeight_));
    math::vec3 deltay = currentUp() * - y * length;
    
    center_ += deltax + deltay;
    updateViewMatrix();
}

// Moves the camera towards or away from the center with a speed relative
// to the distance from the center.
void Camera::zoom(float deltaz)
{
    eyeDir_ = eyeDir_ * (-deltaz * zoomSpeed_ / windowHeight_ + 1);
    updateViewMatrix();
}


// ========================= MATRIX GETTER =============================== 

// Returns the OpenGL formatted view matrix
const float * Camera::getViewMatrix()
{
    return viewMatrix_.get();
}

// ========================= INTERNAL CAMERA UPDATING ====================

// Updates the view matrix:
// * Computes the view rotation for the lookat
// * Gets the arcball rotation matrix
// * Computes the following transformation:
//      translate points by -eye (move origin to eye)
//      rotate by lookatRot
//      translate into the view center (translation)
//      rotate with the arcball rotation
//      translate back out
void Camera::updateViewMatrix()
{
    math::vec3 eye = center_ - eyeDir_;
    
    // Get the lookat matrices
    math::mat4 lookatRot(center_,eye, math::vec3(0,1,0));
    math::mat4 lookAtTranslation(1,0,0,-eye[0],
                     0,1,0,-eye[1],
                     0,0,1,-eye[2],
                     0,0,0,1);
    
    // Create the translation matrices into and out of the view center
    math::mat4 translation = math::mat4(1,0,0,0,
                                        0,1,0,0,
                                        0,0,1,eyeDir_.length(),
                                        0,0,0,1);
    
    math::mat4 invtranslation = math::mat4(1,0,0,0,
                                        0,1,0,0,
                                        0,0,1,-eyeDir_.length(),
                                        0,0,0,1);
    
    math::mat4 arcRot = arcBall_.get_mat();
    
    viewMatrix_ =  (invtranslation * arcRot.transpose() * translation  * lookatRot * lookAtTranslation).transpose();
}

// Gets the up vector of the current view plane
math::vec3 Camera::currentUp()
{
    math::vec4 up = viewMatrix_.colvec(1);
    return math::vec3(up[0],up[1],up[2]);
}

// Gets the right vector of the current view plane
math::vec3 Camera::currentRight()
{
    math::vec4 right = viewMatrix_.colvec(0);
    return math::vec3(right[0],right[1],right[2]);
}



