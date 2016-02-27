#pragma once

#include "Camera.h"

#include "Game.h"

namespace krt
{

struct EditorCameraControls
{
    EditorCameraControls( void );
    ~EditorCameraControls( void );

    void OnFrame( Camera *camera );

    void SetAccelerationTime( unsigned int time );

    // Velocity manipulation functions.
    void AddRightVelocity( float velo )     { this->right_velo.GiveVelocity( this->right_velo.GetCurrent() + velo ); }
    void SetRightVelocity( float velo )     { this->right_velo.GiveVelocity( velo ); }
    float GetRightVelocity( void )          { return this->right_velo.GetCurrent(); }
    void StopRightVelocity( void )          { this->right_velo.StopMovement(); }

    void AddFrontVelocity( float velo )     { this->front_velo.GiveVelocity( this->front_velo.GetCurrent() + velo ); }
    void SetFrontVelocity( float velo )     { this->front_velo.GiveVelocity( velo ); }
    float GetFrontVelocity( void )          { return this->front_velo.GetCurrent(); }
    void StopFrontVelocity( void )          { this->front_velo.StopMovement(); }

    void AddUpVelocity( float velo )        { this->up_velo.GiveVelocity( this->up_velo.GetCurrent() + velo ); }
    void SetUpVelocity( float velo )        { this->up_velo.GiveVelocity( velo ); }
    float GetUpVelocity( void )             { return this->up_velo.GetCurrent(); }
    void StopUpVelocity( void )             { this->up_velo.StopMovement(); }

    void AddPitchVelocity( float velo )     { this->pitch_velo.GiveVelocity( this->pitch_velo.GetCurrent() + velo ); }
    void SetPitchVelocity( float velo )     { this->pitch_velo.GiveVelocity( velo ); }
    float GetPitchVelocity( void )          { return this->pitch_velo.GetCurrent(); }
    void StopPitchVelocity( void )          { this->pitch_velo.StopMovement(); }

    void AddYawVelocity( float velo )     { this->yaw_velo.GiveVelocity( this->yaw_velo.GetCurrent() + velo ); }
    void SetYawVelocity( float velo )     { this->yaw_velo.GiveVelocity( velo ); }
    float GetYawVelocity( void )          { return this->yaw_velo.GetCurrent(); }
    void StopYawVelocity( void )          { this->yaw_velo.StopMovement(); }

    void StopVelocity( void )
    {
        this->right_velo.StopMovement();
        this->front_velo.StopMovement();
        this->up_velo.StopMovement();

        this->yaw_velo.StopMovement();
        this->pitch_velo.StopMovement();
    }

private:
    typedef uint64_t timer_t;

    template <typename numberType>
    struct InterpolatedVelocity
    {
        inline InterpolatedVelocity( void )
        {
            this->isMoving = false;
            this->isStopping = false;

            this->acceleration_time = 1000;
        }

        inline void GiveVelocity( float speed )
        {
            timer_t cur_milli = theGame->GetGameTime();

            this->velocity_at_start_request = this->GetAtTime( cur_milli );
            this->start_velocity = cur_milli;

            this->isStopping = false;
            this->isMoving = true;

            this->requested_velocity = speed;
        }

        inline void StopMovement( void )
        {
            if ( this->isMoving )
            {
                // Update current status.
                timer_t cur_milli = theGame->GetGameTime();

                this->velocity_at_start_request = this->GetAtTime( cur_milli );
                this->start_velocity = cur_milli;

                this->isStopping = true;
                this->isMoving = false;
            }
        }

        inline void SetAccelerationInterval( timer_t interval_ms )
        {
            assert( isMoving == false && isStopping == false );

            this->acceleration_time = interval_ms;

            // TODO: update the current velocity.
        }

        inline numberType GetCurrent( void )
        {
            timer_t cur_milli = theGame->GetGameTime();

            numberType currentVelo = this->GetAtTime( cur_milli );

            return currentVelo;
        }

    private:
        inline numberType GetElapsedInterval( timer_t milli ) const
        {
            return std::min( (numberType)1.0, ( (numberType)( milli - this->start_velocity ) / (numberType)acceleration_time ) );
        }

        inline numberType GetAtTime( timer_t cur_milli )
        {
            if ( this->isMoving )
            {
                const numberType elapsed_interval = GetElapsedInterval( cur_milli );

                numberType velocity_to_add = ( this->requested_velocity - this->velocity_at_start_request ) * elapsed_interval;

                return ( this->velocity_at_start_request + velocity_to_add );
            }
            else if ( this->isStopping )
            {
                const numberType elapsed_interval = GetElapsedInterval( cur_milli );

                return ( this->velocity_at_start_request * ( (numberType)1.0 - elapsed_interval ) );
            }

            return 0;
        }

        numberType velocity_at_start_request;
        numberType requested_velocity;

        bool isMoving;
        bool isStopping;
        timer_t start_velocity;
        timer_t acceleration_time;
    };

    InterpolatedVelocity <float> right_velo;
    InterpolatedVelocity <float> front_velo;
    InterpolatedVelocity <float> up_velo;

    InterpolatedVelocity <float> yaw_velo;
    InterpolatedVelocity <float> pitch_velo;
};

}