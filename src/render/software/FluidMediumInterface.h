#ifndef FLUIDMEDIUMINTERFACE_H
#define FLUIDMEDIUMINTERFACE_H

#include "software_global.h"

namespace paysages {
namespace software {

/*!
 * \brief Interface to a fluid medium compatible class.
 */
class SOFTWARESHARED_EXPORT FluidMediumInterface
{
public:
    /*!
     * Return true if the object may change the fluid medium on the given segment.
     * When returning true, the object may alter 'segment' to limit its influence.
     */
    virtual bool checkInfluence(SpaceSegment& segment) const = 0;
};

}
}

#endif // FLUIDMEDIUMINTERFACE_H
