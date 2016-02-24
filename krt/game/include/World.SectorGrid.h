#pragma once

// Infinite world grid to place static world entities on for streaming optimization.

#include <utils/NestedLList.h>
#include "QuadTree.h"

#include "Entity.h"

#include "WorldMath.h"

namespace krt
{

// Each SectorDataType must provide a special interface.

template <typename SectorDataType, size_t sectorBound, size_t sectorQuadTreeDepth>
struct SectorGrid
{
    // Remember that we want to put the center of sector as point (0, 0, 0).
	// d-do we? this breaks anything at the edge of the frustum
    //static constexpr size_t _put_offset = ( sectorBound / 2 );
	static constexpr size_t _put_offset = 0;

    inline SectorGrid( void )
    {
        LIST_CLEAR( this->sectorList.root );
    }

    inline ~SectorGrid( void )
    {
        // Remove all allocated sectors.
        {
            while ( !LIST_EMPTY( this->sectorList.root ) )
            {
                Sector *sector = LIST_GETITEM( Sector, this->sectorList.root.next, node );

                LIST_REMOVE( sector->node );

                delete sector;
            }
        }
    }

    inline void ClearSectors( void )
    {
        // We clear contents of all of our sectors.
        LIST_FOREACH_BEGIN( Sector, this->sectorList.root, node )

            // Notify the data to clear itself.
            item->content.ForAllEntries(
                []( SectorDataEntry& sectorData )
            {
                sectorData.Clear();
            });

        LIST_FOREACH_END
    }

    template <typename callbackType, typename frustumType>
    inline void VisitSectorsByFrustum( const frustumType& frustum, callbackType& cb ) const
    {
        // For each visible sector, actually call our callback.
        LIST_FOREACH_BEGIN( Sector, this->sectorList.root, node )
            
            if ( frustum.intersectWith(item->sectorQuader) )
            {
                // This sector is visible, that means we have to check for visible sub data entries.
                item->content.root.ForAllEntries(
                    [&]( Sector::SectorDataEntry& sectorData )
                {
                    if ( sectorData.IsValid() )
                    {
                        if ( frustum.intersectWith(sectorData.entryQuader) )
                        {
                            // This area on the map is visible, so lets give it to the callback.
                            cb( sectorData.data );
                        }
                    }
                });
            }

        LIST_FOREACH_END
    }

    inline void PutEntity( Entity *entity )
    {
        // When putting an entity on the world grid, we cast a bounding box around it and check which sectors it actually intersects with.
        // We could use a circle for that, but that is for later.

        rw::Sphere boundSphere;

        bool hasEntityBounds = entity->GetWorldBoundingSphere( boundSphere );

        if ( !hasEntityBounds )
            return; // no bounds means no world presence.

        // Create a bounding box.
        float bbox_min_x = boundSphere.center.x - boundSphere.radius;
        float bbox_min_y = boundSphere.center.y - boundSphere.radius;
        float bbox_max_x = boundSphere.center.x + boundSphere.radius;
        float bbox_max_y = boundSphere.center.y + boundSphere.radius;

        // Make sure we allocated all sectors that could be used to put this entity on.
        AllocateSectorInWorldBounds( bbox_min_x, bbox_min_y, bbox_max_x, bbox_max_y );

        // Check for all sectors that intersect this bounding box, 2D style.
        LIST_FOREACH_BEGIN( Sector, this->sectorList.root, node )

            bool hasUpdatedSector = false;

            // Localize the bounding box coordinates to this sector.
            rw::V2d local_bbox_min = item->TranslateCoordinateToSector( rw::V2d( bbox_min_x, bbox_min_y ) );
            rw::V2d local_bbox_max = item->TranslateCoordinateToSector( rw::V2d( bbox_max_x, bbox_max_y ) );

            item->content.root.VisitByBounds( local_bbox_min.x, local_bbox_min.y, local_bbox_max.x, local_bbox_max.y,
                [&]( Sector::SectorDataEntry& sectorData )
            {
                // We found a sector that is interrested in us.
                // That means that we should add it to this sector and recalculate its bounds.

                sectorData.AddSphereToSectorAwareness( boundSphere );
                sectorData.AddEntity( entity );

                hasUpdatedSector = true;
            });

            if ( hasUpdatedSector )
            {
                // Also update parent sector.
                item->UpdateBounds();
            }

        LIST_FOREACH_END
    }

private:
    static MATH_INLINE math::Quader make_sector_quader(
        float bbox_min_x, float bbox_min_y, float bbox_max_x, float bbox_max_y, float min_z, float max_z
    )
    {
        return math::Quader(
            // bottom plane.
            rw::V3d( bbox_min_x, bbox_min_y, min_z ),
            rw::V3d( bbox_max_x, bbox_min_y, min_z ),
            rw::V3d( bbox_min_x, bbox_max_y, min_z ),
            rw::V3d( bbox_max_x, bbox_max_y, min_z ),
            // top plane.
            rw::V3d( bbox_min_x, bbox_min_y, max_z ),
            rw::V3d( bbox_max_x, bbox_min_y, max_z ),
            rw::V3d( bbox_min_x, bbox_max_y, max_z ),
            rw::V3d( bbox_max_x, bbox_max_y, max_z )
        );
    }

    static inline rw::V2d NormalizeCoord( rw::V2d coord )
    {
        return rw::add( coord, rw::V2d( (float)_put_offset, (float)_put_offset ) );
    }

    static inline rw::V2d DenormalizeCoord( rw::V2d coord )
    {
        return rw::sub( coord, rw::V2d( (float)_put_offset, (float)_put_offset ) );
    }

    struct Sector
    {
        struct SectorDataEntry
        {
            inline SectorDataEntry( int bbox_min_x, int bbox_min_y, int bbox_max_x, int bbox_max_y ) : entryQuader( make_sector_quader( (float)bbox_min_x, (float)bbox_min_y, (float)bbox_max_x, (float)bbox_max_y, -9999.0f, 9999.0f ) )
            {
                this->cachedSectorMaxZ = -9999;
                this->cachedSectorMinZ = 9999;

                // Store our bounding box parameters.
                this->bbox_min_x = (float)bbox_min_x;
                this->bbox_min_y = (float)bbox_min_y;
                this->bbox_max_x = (float)bbox_max_x;
                this->bbox_max_y = (float)bbox_max_y;

                this->ownerSector = NULL;
            }

            inline ~SectorDataEntry( void )
            {
                // Clear the entities that reside on us.
                this->Clear();
            }

            inline bool IsValid( void )
            {
                return ( this->cachedSectorMaxZ >= this->cachedSectorMinZ );
            }

            inline void Clear( void )
            {
                // Remove all entities from us.
                {
                    while ( this->presentEntities.empty() == false )
                    {
                        SectorEntityLink *entityLink = this->presentEntities.front();

                        entityLink->Unlink();
                    }
                }

                data.Clear();

                this->RecalculateSectorBounds();
            }

            struct SectorEntityLink : public EntityReference
            {
                inline SectorEntityLink( SectorDataEntry *theEntry, Entity *theEntity )
                {
                    assert( theEntity != NULL );

                    this->theEntry = theEntry;
                    this->addedEntity = theEntity;

                    theEntity->AddEntityWorldSectorReference( this );
                }

                void Unlink( void ) override
                {
                    addedEntity->RemoveEntityWorldReference( this );

                    {
                        auto findIter = std::find( theEntry->presentEntities.begin(), theEntry->presentEntities.end(), this );

                        assert( findIter != theEntry->presentEntities.end() );

                        theEntry->presentEntities.erase( findIter );
                    }

                    theEntry->data.RemoveEntity( addedEntity );

                    delete this;
                }

                SectorDataEntry *theEntry;

                Entity *addedEntity;
            };

            inline void AddSphereToSectorAwareness( const rw::Sphere& worldSphere )
            {
                // Need to update the min and maximum of this sector data entry and recalculate the quader, if required.
                bool hasUpdatedMinMax = false;

                float attempt_min = worldSphere.center.z - worldSphere.radius;
                float attempt_max = worldSphere.center.z + worldSphere.radius;

                if ( this->cachedSectorMaxZ < attempt_max )
                {
                    this->cachedSectorMaxZ = attempt_max;

                    hasUpdatedMinMax = true;
                }
            
                if ( this->cachedSectorMinZ > attempt_min )
                {
                    this->cachedSectorMinZ = attempt_min;

                    hasUpdatedMinMax = true;
                }

                if ( hasUpdatedMinMax )
                {
                    this->MakeNewQuader();
                }
            }

            inline void AddEntity( Entity *entity )
            {
                // Give this entity to the internal data.
                // It could use it to store it in a more finely-tuned octree.
                data.AddEntity( entity );

                // Create a new entity link for us.
                SectorEntityLink *entityLink = new SectorEntityLink( this, entity );

                this->presentEntities.push_back( entityLink );
            }

            inline void RecalculateSectorBounds( void )
            {
                // Recalculate the real sector max z and min z.

                this->cachedSectorMaxZ = -9999; // invalidate the old.
                this->cachedSectorMinZ = 9999;

                for ( SectorEntityLink *entityLink : this->presentEntities )
                {
                    rw::Sphere worldSphere;

                    Entity *addedEntity = entityLink->addedEntity;

                    bool hasWorldBounds = addedEntity->GetWorldBoundingSphere( worldSphere );

                    if ( hasWorldBounds )
                    {
                        this->AddSphereToSectorAwareness( worldSphere );
                    }
                }

                this->MakeNewQuader();
            }

            inline void MakeNewQuader( void )
            {
                // Have to put the quader in world space.
                rw::V2d world_space_min = ownerSector->TranslateCoordinateToWorld( rw::V2d( this->bbox_min_x, this->bbox_min_y ) );
                rw::V2d world_space_max = ownerSector->TranslateCoordinateToWorld( rw::V2d( this->bbox_max_x, this->bbox_max_y ) );

                this->entryQuader = make_sector_quader(
                    world_space_min.x, world_space_min.y,
                    world_space_max.x, world_space_max.y,
                    this->cachedSectorMinZ, this->cachedSectorMaxZ
                );
            }

            Sector *ownerSector;

            SectorDataType data;

            math::Quader entryQuader;

            // List of all entities that are registered in this sector.
            std::list <SectorEntityLink*> presentEntities;

            // Cached parameters.
            float cachedSectorMaxZ;
            float cachedSectorMinZ;

            // Data about this sector.
            float bbox_min_x, bbox_min_y, bbox_max_x, bbox_max_y;
        };

        inline Sector( float min_x, float min_y ) : sectorQuader( make_sector_quader( min_x, min_y, min_x + sectorBound, min_y + sectorBound, -9999, 9999 ) )
        {
            this->cachedSectorMaxZ = -9999;
            this->cachedSectorMinZ = 9999;

            this->sector_min_x = min_x;
            this->sector_min_y = min_y;
            this->sector_max_x = min_x + sectorBound;
            this->sector_max_y = min_y + sectorBound;

            // Give owner links to the sector.
            this->content.root.ForAllEntries(
                [&]( SectorDataEntry& dataEntry )
            {
                dataEntry.ownerSector = this;
            });
        }

        inline void UpdateBounds( void )
        {
            this->cachedSectorMaxZ = -9999;
            this->cachedSectorMinZ = 9999;

            bool hasUpdatedMinMax = false;

            content.root.ForAllEntries(
                [&]( SectorDataEntry& dataEntry )
            {
                if ( this->cachedSectorMaxZ < dataEntry.cachedSectorMaxZ )
                {
                    this->cachedSectorMaxZ = dataEntry.cachedSectorMaxZ;

                    hasUpdatedMinMax = true;
                }

                if ( this->cachedSectorMinZ > dataEntry.cachedSectorMinZ )
                {
                    this->cachedSectorMinZ = dataEntry.cachedSectorMinZ;

                    hasUpdatedMinMax = true;
                }
            });

            if ( hasUpdatedMinMax )
            {
                // Need to make a new quader.
                // We want to put it in world space.
                this->sectorQuader = make_sector_quader(
                    this->sector_min_x, this->sector_min_y,
                    this->sector_max_x, this->sector_max_y,
                    this->cachedSectorMinZ, this->cachedSectorMaxZ
                );
            }
        }

        inline rw::V2d TranslateCoordinateToSector( rw::V2d coord )
        {
            return rw::sub( NormalizeCoord( coord ), rw::V2d( this->sector_min_x, this->sector_min_y ) );
        }

        inline rw::V2d TranslateCoordinateToWorld( rw::V2d coord )
        {
            return rw::add( DenormalizeCoord( coord ), rw::V2d( this->sector_min_x, this->sector_min_y ) );
        }

        QuadTree <sectorQuadTreeDepth, sectorBound, SectorDataEntry> content;

        NestedListEntry <Sector> node;

        // Cached parameters.
        float cachedSectorMaxZ;
        float cachedSectorMinZ;

        math::Quader sectorQuader;

        // Bounds of this sector.
        // Should not intersect with any other sector.
        float sector_min_x;
        float sector_min_y;
        float sector_max_x;
        float sector_max_y;
    };

    inline float TranslateCoordToSectorMin( float coord )
    {
        return (float)floor( coord / sectorBound ) * sectorBound;
    }

    inline float TranslateCoordToSectorMax( float coord )
    {
        return (float)ceil( coord / sectorBound ) * sectorBound;
    }

    template <typename callbackType>
    inline void ForAllSectorBounds( float min_x, float min_y, float max_x, float max_y, callbackType& cb )
    {
        float sector_scan_min_x = TranslateCoordToSectorMin( min_x );
        float sector_scan_min_y = TranslateCoordToSectorMin( min_y );
        
        float sector_scan_max_x = TranslateCoordToSectorMax( max_x );
        float sector_scan_max_y = TranslateCoordToSectorMax( max_y );

        int num_sectors_width = (int)( ( sector_scan_max_x - sector_scan_min_x ) / sectorBound );
        int num_sectors_height = (int)( ( sector_scan_max_y - sector_scan_min_y ) / sectorBound );

        // We want to look at all sectors between min and max and see if they are allocated.

        for ( int y = 0; y < num_sectors_height; y++ )
        {
            for ( int x = 0; x < num_sectors_width; x++ )
            {
                // Calculate the world coordinates of this sector bounds.
                float _sector_bound_min_x = (float)( sector_scan_min_x + sectorBound * x );
                float _sector_bound_min_y = (float)( sector_scan_min_y + sectorBound * y );

                // Normalize the coordinates.
                rw::V2d norm_sector_bound_min = NormalizeCoord( rw::V2d( _sector_bound_min_x, _sector_bound_min_y ) );

                cb( norm_sector_bound_min.x, norm_sector_bound_min.y, norm_sector_bound_min.x + sectorBound, norm_sector_bound_min.y + sectorBound );
            }
        }
    }

    Sector* FindSectorForBounds( float world_sector_min_x, float world_sector_min_y, float world_sector_max_x, float world_sector_max_y )
    {
        LIST_FOREACH_BEGIN( Sector, this->sectorList.root, node )

            // It is enough to check position.
            if ( item->sector_min_x == world_sector_min_x && item->sector_min_y == world_sector_min_y )
            {
                return item;
            }

        LIST_FOREACH_END

        return NULL;
    }

    void AllocateSectorInWorldBounds( float min_x, float min_y, float max_x, float max_y )
    {
        // If there is a world entity, we need the sector.

        ForAllSectorBounds( min_x, min_y, max_x, max_y,
            [&]( float sector_min_x, float sector_min_y, float sector_max_x, float sector_max_y )
        {
            // See if there is a sector that maps to these bounds.
            // If there isnt any such sector, then we should allocate one.
            Sector *sectorAtPos = FindSectorForBounds( sector_min_x, sector_min_y, sector_max_x, sector_max_y );

            if ( !sectorAtPos )
            {
                // Do the allocation.
                Sector *newSector = new Sector( sector_min_x, sector_min_y );

                LIST_INSERT( this->sectorList.root, newSector->node );
            }
        });
    }

    NestedList <Sector> sectorList;
};

}