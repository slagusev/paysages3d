#include "BaseTestCase.h"

#include "SpaceGridIterator.h"
#include "SpaceSegment.h"

class CollectGridIterator : public SpaceGridIterator {
  public:
    vector<Vector3> locations;

  protected:
    virtual bool onCell(int x, int y, int z) override {
        locations.push_back(Vector3(x, y, z));
        return true;
    }
};

TEST(SpaceSegment, iterateOnGrid) {
    CollectGridIterator it;
    SpaceSegment segment(Vector3(0.5, 1.5, 0.0), Vector3(2.5, 0.5, 0.0));
    segment.iterateOnGrid(it);

    ASSERT_EQ(4, (int)it.locations.size());
    EXPECT_VECTOR3_COORDS(it.locations[0], 0.0, 1.0, 0.0);
    EXPECT_VECTOR3_COORDS(it.locations[1], 1.0, 1.0, 0.0);
    EXPECT_VECTOR3_COORDS(it.locations[2], 1.0, 0.0, 0.0);
    EXPECT_VECTOR3_COORDS(it.locations[3], 2.0, 0.0, 0.0);
}

TEST(SpaceSegment, iterateOnGrid_Corner) {
    CollectGridIterator it;
    SpaceSegment segment(Vector3(0.5, 0.5, 0.5), Vector3(2.5, 2.5, 2.5));
    segment.iterateOnGrid(it);

    ASSERT_EQ(7, (int)it.locations.size());
    EXPECT_VECTOR3_COORDS(it.locations[0], 0.0, 0.0, 0.0);
    EXPECT_VECTOR3_COORDS(it.locations[3], 1.0, 1.0, 1.0);
    EXPECT_VECTOR3_COORDS(it.locations[6], 2.0, 2.0, 2.0);
}

TEST(SpaceSegment, iterateOnGrid_OneCell) {
    CollectGridIterator it;
    SpaceSegment segment(Vector3(8.1, 8.2, 8.9), Vector3(8.9, 8.3, 8.6));
    segment.iterateOnGrid(it);

    ASSERT_EQ(1, (int)it.locations.size());
    EXPECT_VECTOR3_COORDS(it.locations[0], 8.0, 8.0, 8.0);
}

TEST(SpaceSegment, iterateOnGrid_Negative) {
    CollectGridIterator it;
    SpaceSegment segment(Vector3(-8.1, -8.2, -8.9), Vector3(-8.9, -8.3, -8.6));
    segment.iterateOnGrid(it);

    ASSERT_EQ(1, (int)it.locations.size());
    EXPECT_VECTOR3_COORDS(it.locations[0], -9.0, -9.0, -9.0);
}

TEST(SpaceSegment, intersectBoundingBox) {
    SpaceSegment bbox(Vector3(-1.0, -1.0, -1.0), Vector3(1.0, 4.0, 1.0));

    EXPECT_TRUE(SpaceSegment(Vector3(0.0, 0.0, 0.0), Vector3(1.0, 1.0, 1.0)).intersectBoundingBox(bbox));

    EXPECT_TRUE(SpaceSegment(Vector3(-5.0, 0.0, 0.0), Vector3(5.0, 0.0, 0.0)).intersectBoundingBox(bbox));
    EXPECT_FALSE(SpaceSegment(Vector3(-5.0, -2.0, 0.0), Vector3(5.0, -2.0, 0.0)).intersectBoundingBox(bbox));

    EXPECT_TRUE(SpaceSegment(Vector3(-5.0, 0.0, -5.0), Vector3(5.0, 0.0, 5.0)).intersectBoundingBox(bbox));
    EXPECT_FALSE(SpaceSegment(Vector3(-5.0, -2.0, 0.0), Vector3(5.0, -2.0, 5.0)).intersectBoundingBox(bbox));

    EXPECT_FALSE(SpaceSegment(Vector3(-10.0, 0.0, 0.0), Vector3(10.0, -6.0, 0.0)).intersectBoundingBox(bbox));
}
