#include <gtest/gtest.h>

#include <set>
#include <vector>

#include <si_flat_hash_map.h>


TEST(ProbeSeq, visitAll)
{
    // Simulate having 31 control slots split into 4 groups
    int capacity = 31;
    std::vector<si::ctrl_t> ctrl(capacity);
    size_t hash = std::hash<double>{}(3.14);
    std::set<size_t> probes;

    si::ProbeSeq<si::Group::width> seq(si::H1(hash, ctrl.data()), capacity);
    probes.insert(seq.offset());
    for (int i = 1; i < 4; i ++)
    {
        seq.next();
        probes.insert(seq.offset());
    }
    // All 4 groups should be visited from the first pass.
    EXPECT_EQ(probes.size(), 4);

    for (int i = 4; i < 20; i ++)
    {
        seq.next();
        probes.insert(seq.offset());
    }
    // Check we keep probing to the same initial 4 groups.
    EXPECT_EQ(probes.size(), 4);
}


TEST(BitMask, iteration)
{
    std::vector<int> set_bits;
    // 0x80 = 0b10000000
    for (int i : si::BitMask<uint64_t>(0x0000000080800000ULL))
        set_bits.push_back(i);

    EXPECT_EQ(set_bits, std::vector({2, 3}));
}

TEST(BitMask, lowestSetBit)
{
    si::BitMask<uint64_t> bm(0x0000000080800000ULL);

    EXPECT_EQ(bm.lowestSetBit(), 2);
}

TEST(Group, match)
{
    uint8_t h2 = 5;
    std::vector<si::ctrl_t> ctrl { 5, 5, 0, 0, 0, 0, 0, 0 };

    si::Group g(ctrl.data());
    std::vector<int> set_bits;
    for (int i : g.match(h2))
        set_bits.push_back(i);

    // Test that we load the bytes as little endian.
    EXPECT_EQ(g.ctrl, 0x0000000000000505ULL);
    EXPECT_EQ(set_bits, std::vector({0, 1}));
}

TEST(Group, matchEmpty)
{
    // Empty buckets are marked with -128 = 0x80 = 0b10000000.
    // Sentinels are marked with -1.
    std::vector<si::ctrl_t> ctrl { 0, -128, -128, -1, 0, 0, 7, 8 };

    si::Group g(ctrl.data());
    std::vector<int> set_bits;
    for (int i : g.matchEmpty())
        set_bits.push_back(i);

    EXPECT_EQ(g.ctrl, 0x08070000FF808000ULL);
    EXPECT_EQ(set_bits, std::vector({1, 2}));
}

TEST(Group, matchEmptyOrDeleted)
{
    // Empty buckets are marked with -128 = 0x80 = 0b10000000.
    // Deleted buckets are maked with  -2 = 0xFE = 0b11111110.
    // Sentinels are marked with       -1 = 0xFF = 0b11111111.
    std::vector<si::ctrl_t> ctrl { 0, -2, -1, -128, -2, 0, 0, 0 };

    si::Group g(ctrl.data());
    std::vector<int> set_bits;
    for (int i : g.matchEmptyOrDeleted())
        set_bits.push_back(i);

    EXPECT_EQ(g.ctrl, 0x000000FE80FFFE00ULL);
    EXPECT_EQ(set_bits, std::vector({1, 3, 4}));
}


TEST(Group, countLeadingEmptyOrDeleted)
{
    std::vector<si::ctrl_t> ctrl0 {  0,    0, 0, 0, -128, 0, 0, 0 };
    std::vector<si::ctrl_t> ctrl1 { -2,    0, 0, 0, -128, 0, 0, 0 };
    std::vector<si::ctrl_t> ctrl2 { -2, -128, 0, 0, -128, 0, 0, 0 };

    si::Group g0(ctrl0.data());
    si::Group g1(ctrl1.data());
    si::Group g2(ctrl2.data());

    EXPECT_EQ(g0.countLeadingEmptyOrDeleted(), 0);
    EXPECT_EQ(g1.countLeadingEmptyOrDeleted(), 1);
    EXPECT_EQ(g2.countLeadingEmptyOrDeleted(), 2);
}

TEST(Group, convertSpecialToEmptyAndFullToDeleted)
{
    // DELETED -2   / 0xFE       -> EMPTY:   0x80
    // EMPTY   -128 / 0x80       -> EMPTY:   0x80
    // FULL    >=0  / 0b0xxxxxxx -> DELETED: 0xFE
    std::vector<si::ctrl_t> ctrl { 0, 0, -2, -2, -128, 6, 7, 0 };
    EXPECT_EQ(boost::endian::native_to_little(*(uint64_t*)(ctrl.data())), 0x00070680FEFE0000ULL);

    si::Group g(ctrl.data());
    g.convertSpecialToEmptyAndFullToDeleted(ctrl.data());

    EXPECT_EQ(boost::endian::native_to_little(*(uint64_t*)(ctrl.data())), 0xFEFEFE808080FEFEULL);
}

