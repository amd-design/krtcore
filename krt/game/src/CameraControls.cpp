#include "StdInc.h"
#include "CameraControls.h"

#include "Game.h"

// Simple camera movement classes for map viewer and things.

namespace krt
{
    
EditorCameraControls::EditorCameraControls( void )
{
    // Do some tests.
}

EditorCameraControls::~EditorCameraControls( void )
{
    return;
}

static float normalize_angle( float angle )
{
    if ( isnan( angle ) || isinf( angle ) )
        return 0;

    while ( angle < 0 )
    {
        angle += 360;
    }

    while ( angle >= 360 )
    {
        angle -= 360;
    }

    return angle;
}

// A little messy but hopefully it works.

inline void matrix_get_yaw_pitch( const rw::Matrix& viewMat, float& yaw_out, float& pitch_out )
{
    float xy_length = rw::length( rw::V3d( viewMat.at.x, viewMat.at.y, 0.0f ) );

    float z_front = viewMat.at.z;

    if ( z_front > 0.99999 )
    {
        pitch_out = 90;
    }
    else if ( z_front < -0.99999 )
    {
        pitch_out = -90;
    }
    else
    {
        pitch_out = (float)RAD2DEG( atan2( z_front, xy_length ) );
    }

    if ( abs(z_front) > 0.99999 )
    {
        if ( viewMat.up.y < 0 )
        {
            yaw_out = 270 - (float)RAD2DEG( atan2( viewMat.up.y, viewMat.up.x ) );
        }
        else
        {
            yaw_out = 270 - (float)RAD2DEG( atan2( viewMat.up.y, viewMat.up.x ) );
        }
    }
    else
    {
        if ( viewMat.at.y < 0 )
        {
            yaw_out = 90 - (float)RAD2DEG( atan2( viewMat.at.y, viewMat.at.x ) );
        }
        else
        {
            yaw_out = 90 - (float)RAD2DEG( atan2( viewMat.at.y, viewMat.at.x ) );
        }
    }

    yaw_out = normalize_angle( yaw_out );
}

inline void matrix_from_yaw_pitch( rw::Matrix& matOut, float yaw, float pitch )
{
    double yaw_rad = DEG2RAD( yaw );
    double pitch_rad = DEG2RAD( pitch );

    float at_vec_x_pos = (float)sin( yaw_rad );
    float at_vec_y_pos = (float)cos( yaw_rad );

    float at_vec_height = (float)sin( pitch_rad );
    float at_xy_vec_length = (float)cos( pitch_rad );

    rw::V3d at_vector = rw::normalize( rw::V3d( at_vec_x_pos * at_xy_vec_length, at_vec_y_pos * at_xy_vec_length, at_vec_height ) );

    rw::V3d up_vector;
    rw::V3d right_vector;

    if ( at_vector.z == 1 )
    {
        up_vector = rw::V3d( -at_vec_x_pos, -at_vec_y_pos, 0 );

        right_vector = rw::normalize( rw::cross( at_vector, up_vector ) );
    }
    else if ( at_vector.z == -1 )
    {
        up_vector = rw::V3d( at_vec_x_pos, at_vec_y_pos, 0 );

        right_vector = rw::normalize( rw::cross( at_vector, up_vector ) );
    }
    else
    {
        rw::V3d normalizator( 0, 0, 1 );

        right_vector = rw::normalize( rw::cross( normalizator, at_vector ) );
        up_vector = rw::normalize( rw::cross( at_vector, right_vector ) );
    }

    matOut.right = right_vector;
    matOut.rightw = 0;
    matOut.up = up_vector;
    matOut.upw = 0;
    matOut.at = at_vector;
    matOut.atw = 0;
}

void EditorCameraControls::OnFrame( Camera *camera )
{
    // Move the camera.
    rw::Matrix camView = camera->GetViewMatrix();

    float frame_delta = theGame->GetDelta();

    // Linear velocity.
    rw::V3d linear_velo( this->right_velo.GetCurrent(), this->up_velo.GetCurrent(), this->front_velo.GetCurrent() );

    rw::V3d frame_linear_velo = rw::scale( linear_velo, frame_delta );

    rw::V3d new_cam_pos = camView.transPoint( frame_linear_velo );

    camView.pos = new_cam_pos;

    // Linear turn velocity, based on euler angles.
    {
        float yaw_velo_val = this->yaw_velo.GetCurrent() * frame_delta;
        float pitch_velo_val = this->pitch_velo.GetCurrent() * frame_delta;

        if ( yaw_velo_val != 0 || pitch_velo_val != 0 )
        {
            rw::Matrix cur_frame_rotate;
            cur_frame_rotate.setIdentity();

            float cur_yaw, cur_pitch;
            matrix_get_yaw_pitch( camView, cur_yaw, cur_pitch );

            // Make sure we do not go overboard, so limit pitch.
            float new_pitch = std::max( -89.0f, std::min( 89.0f, (float)( cur_pitch + pitch_velo_val ) ) );

            matrix_from_yaw_pitch( camView, cur_yaw + yaw_velo_val, new_pitch );
        }
    }

    camera->SetViewMatrix( camView );
}

void EditorCameraControls::AddViewAngles( Camera* camera, float yaw, float pitch )
{
	rw::Matrix camView = camera->GetViewMatrix();

	if (yaw != 0 || pitch != 0)
	{
		float cur_yaw, cur_pitch;
		matrix_get_yaw_pitch(camView, cur_yaw, cur_pitch);

		// Make sure we do not go overboard, so limit pitch.
		float new_pitch = std::max(-89.0f, std::min(89.0f, (float)(cur_pitch + pitch)));

		matrix_from_yaw_pitch(camView, cur_yaw + yaw, new_pitch);
	}

	camera->SetViewMatrix(camView);
}

void EditorCameraControls::SetAccelerationTime( unsigned int time )
{
    this->front_velo.SetAccelerationInterval( time );
    this->right_velo.SetAccelerationInterval( time );
    this->up_velo.SetAccelerationInterval( time );
}

};