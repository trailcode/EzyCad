#include "sketch_test_fixture.h"

#include <set>
#include <string>
#include <vector>

#include "sketch_edge.h"
#include "sketch_json.h"
#include "sketch_nodes.h"
#include "utl_geom.h"
#include "utl_asset_store.h"
#include "utl_io.h"

using namespace glm;

namespace
{
nlohmann::json minimal_sketch_json_with_underlay_b64(Ezy_asset_store& store);
} // namespace

// Test JSON serialization and deserialization
TEST_F(Sketch_test, JsonSerializationDeserialization)
{
  gp_Pln default_plane(gp::Origin(), gp::DZ());
  Sketch sketch("TestSketch", view(), default_plane);

  // Add some edges to create a simple shape
  std::vector<gp_Pnt2d> points = {
      gp_Pnt2d(-42.123413069225286, 18.567557076566406), gp_Pnt2d(-31.038304366797583, 18.567557076566406),
      gp_Pnt2d(-42.123413069225286, 42.585292598493105), gp_Pnt2d(-31.038304366797583, 42.585292598493105),
      gp_Pnt2d(-42.123413069225286, -5.450178445360293), gp_Pnt2d(-31.038304366797583, -5.450178445360293)};

  // Add edges to create a rectangle-like shape
  for (size_t i = 0; i < points.size() - 1; i += 2)
    Sketch_access::add_edge_(sketch, points[i], points[i + 1]);

  // Serialize to JSON
  nlohmann::json json_data = Sketch_json::to_json(sketch, view().asset_store());

  // Verify JSON structure
  EXPECT_TRUE(json_data.contains("name"));
  EXPECT_TRUE(json_data.contains("edges"));
  EXPECT_TRUE(json_data.contains("arc_edges"));
  EXPECT_TRUE(json_data.contains("plane"));
  EXPECT_TRUE(json_data.contains("isCurrent"));
  EXPECT_TRUE(json_data.contains("length_dimensions"));
  EXPECT_EQ(json_data["length_dimensions"].size(), 0u);

  EXPECT_EQ(json_data["name"], "TestSketch");
  EXPECT_TRUE(json_data.contains("nodes"));
  size_t live_nodes = 0;
  for (size_t i = 0; i < sketch.get_nodes().size(); ++i)
    if (!sketch.get_nodes()[i].deleted)
      ++live_nodes;

  EXPECT_EQ(json_data["nodes"].size(), live_nodes);
  EXPECT_EQ(json_data["edges"].size(), 3);     // Should have 3 edges
  EXPECT_EQ(json_data["arc_edges"].size(), 0); // No arc edges

  // Deserialize from JSON
  std::shared_ptr<Sketch> deserialized_sketch = Sketch_json::from_json(view(), json_data);

  // Verify deserialized sketch (compact save drops deleted tombstones; loaded sketch is dense)
  EXPECT_EQ(deserialized_sketch->get_name(), "TestSketch");
  EXPECT_EQ(deserialized_sketch->get_nodes().size(), live_nodes);

  // Count edges in deserialized sketch
  EXPECT_EQ(Sketch_access::get_edge_count(*deserialized_sketch), 3); // Should have 3 edges
}

// Test JSON serialization with different edge counts (bug1 vs bug1.1 scenario)

// Test JSON serialization with different edge counts (bug1 vs bug1.1 scenario)
TEST_F(Sketch_test, JsonSerializationDifferentEdgeCounts)
{
  gp_Pln default_plane(gp::Origin(), gp::DZ());

  // Create first sketch with 3 edges (like bug1.ezy)
  Sketch                sketch1("Sketch1", view(), default_plane);
  std::vector<gp_Pnt2d> points1 = {
      gp_Pnt2d(-42.123413069225286, 18.567557076566406), gp_Pnt2d(-31.038304366797583, 18.567557076566406),
      gp_Pnt2d(-42.123413069225286, 42.585292598493105), gp_Pnt2d(-31.038304366797583, 42.585292598493105),
      gp_Pnt2d(-42.123413069225286, -5.450178445360293), gp_Pnt2d(-31.038304366797583, -5.450178445360293)};

  // Add 3 edges using raw (bypasses auto-split of crossing/touching; produces exactly the edge count
  // present in the original bug report files for this serialization test).
  for (size_t i = 0; i < points1.size() - 1; i += 2)
  {
    Sketch_access::add_edge_raw_(sketch1, points1[i], points1[i + 1]);
  }

  // Create second sketch with 4 edges (like bug1.1.ezy)
  Sketch                sketch2("Sketch2", view(), default_plane);
  std::vector<gp_Pnt2d> points2 = {
      gp_Pnt2d(-42.123413069225286, 18.567557076566406), gp_Pnt2d(-31.038304366797583, 18.567557076566406),
      gp_Pnt2d(-42.123413069225286, 42.585292598493105), gp_Pnt2d(-31.038304366797583, 42.585292598493105),
      gp_Pnt2d(-42.123413069225286, -5.450178445360293), gp_Pnt2d(-31.038304366797583, -5.450178445360293),
      gp_Pnt2d(-42.123413069225286, -5.450178445360293), gp_Pnt2d(-42.123413069225286, 42.585292598493105)};

  // Add 4 edges (including the vertical edge) using raw (bypasses auto-split of crossing/touching; produces
  // exactly the edge count present in the original bug report files for this serialization test).
  for (size_t i = 0; i < points2.size() - 1; i += 2)
  {
    Sketch_access::add_edge_raw_(sketch2, points2[i], points2[i + 1]);
  }

  // Serialize both sketches
  nlohmann::json json1 = Sketch_json::to_json(sketch1, view().asset_store());
  nlohmann::json json2 = Sketch_json::to_json(sketch2, view().asset_store());

  // Verify different edge counts
  EXPECT_EQ(json1["edges"].size(), 3);
  EXPECT_EQ(json2["edges"].size(), 4);

  // Deserialize and verify edge counts are preserved
  std::shared_ptr<Sketch> deserialized1 = Sketch_json::from_json(view(), json1);
  std::shared_ptr<Sketch> deserialized2 = Sketch_json::from_json(view(), json2);

  size_t edge_count1 = Sketch_access::get_edge_count(*deserialized1);
  size_t edge_count2 = Sketch_access::get_edge_count(*deserialized2);

  EXPECT_EQ(edge_count1, 3);
  EXPECT_EQ(edge_count2, 4);
  EXPECT_NE(edge_count1, edge_count2);
}

// Test JSON serialization with length dimensions (node pairs, not per-edge)

// Test JSON serialization with length dimensions (node pairs, not per-edge)
TEST_F(Sketch_test, JsonSerializationWithDimensions)
{
  gp_Pln default_plane(gp::Origin(), gp::DZ());
  Sketch sketch("TestSketch", view(), default_plane);

  gp_Pnt2d pt1(-42.123413069225286, 18.567557076566406);
  gp_Pnt2d pt2(-31.038304366797583, 18.567557076566406);

  Sketch_access::add_edge_(sketch, pt1, pt2);

  nlohmann::json json_data = Sketch_json::to_json(sketch, view().asset_store());
  // Endpoints remap to dense indices 0 and 1 (midpoint is 2)
  json_data["length_dimensions"] = nlohmann::json::array({nlohmann::json::array({0, 1})});

  EXPECT_EQ(json_data["edges"].size(), 1u);
  EXPECT_EQ(json_data["edges"][0].size(), 3u);

  std::shared_ptr<Sketch> deserialized_sketch = Sketch_json::from_json(view(), json_data);

  ASSERT_EQ(Sketch_access::length_dimension_count(*deserialized_sketch), 1u);
  EXPECT_EQ(Sketch_access::length_dimension_node_lo(*deserialized_sketch, 0), 0u);
  EXPECT_EQ(Sketch_access::length_dimension_node_hi(*deserialized_sketch, 0), 1u);
}

// Legacy indexed edges used a 4th boolean "dim" field; migrate to length_dimensions (node pair).

// Legacy indexed edges used a 4th boolean "dim" field; migrate to length_dimensions (node pair).
TEST_F(Sketch_test, JsonLegacyIndexedEdgeDimFlagMigratesToLengthDimensions)
{
  gp_Pln default_plane(gp::Origin(), gp::DZ());
  Sketch sketch("TestSketch", view(), default_plane);
  Sketch_access::add_edge_(sketch, gp_Pnt2d(0.0, 0.0), gp_Pnt2d(10.0, 0.0));

  nlohmann::json j = Sketch_json::to_json(sketch, view().asset_store());
  j["edges"][0].push_back(true);
  j.erase("length_dimensions");

  std::shared_ptr<Sketch> loaded = Sketch_json::from_json(view(), j);
  ASSERT_EQ(Sketch_access::length_dimension_count(*loaded), 1u);
  EXPECT_EQ(Sketch_access::length_dimension_node_lo(*loaded, 0), 0u);
  EXPECT_EQ(Sketch_access::length_dimension_node_hi(*loaded, 0), 1u);
}

TEST_F(Sketch_test, EzyDocumentJsonIncludesFormatVersion)
{
  const nlohmann::json doc = nlohmann::json::parse(view().to_json());
  ASSERT_TRUE(doc.contains("ezyFormat"));
  EXPECT_EQ(doc["ezyFormat"].get<int>(), 3);
}

// Underlay round-trip through .ezy zip
TEST_F(Sketch_test, EzyZipUnderlayRoundTrip)
{
  view().asset_store().clear();
  nlohmann::json doc     = nlohmann::json::parse(view().to_json());
  doc["sketches"]        = nlohmann::json::array({minimal_sketch_json_with_underlay_b64(view().asset_store())});
  const std::string load = doc.dump();
  view().load(load, false);

  const auto sk = view().curr_sketch_shared();
  ASSERT_TRUE(sk);
  ASSERT_TRUE(sk->underlay().has_image());
  EXPECT_EQ(sk->underlay().image_w(), 2);
  EXPECT_EQ(sk->underlay().image_h(), 2);

  const std::string          manifest  = view().to_json();
  const std::vector<uint8_t> zip_bytes = pack_ezy(manifest, view().asset_store());
  ASSERT_FALSE(zip_bytes.empty());
  ASSERT_TRUE(is_ezy_zip(std::string(reinterpret_cast<const char*>(zip_bytes.data()), zip_bytes.size())));

  auto unpacked = unpack_ezy(std::string(reinterpret_cast<const char*>(zip_bytes.data()), zip_bytes.size()));
  ASSERT_TRUE(unpacked);
  EXPECT_FALSE(unpacked->manifest_json.empty());
  EXPECT_EQ(unpacked->assets.size(), 1u);

  view().asset_store().clear();
  for (auto& [id, data] : unpacked->assets)
    view().asset_store().import_asset(id, std::move(data));

  view().load(unpacked->manifest_json, false);
  const auto loaded = view().curr_sketch_shared();
  ASSERT_TRUE(loaded);
  EXPECT_TRUE(loaded->underlay().has_image());
  EXPECT_EQ(loaded->underlay().image_w(), 2);
  EXPECT_EQ(loaded->underlay().image_h(), 2);
}

TEST_F(Sketch_test, EzyZipUnderlayDedup)
{
  view().asset_store().clear();
  nlohmann::json sk1 = minimal_sketch_json_with_underlay_b64(view().asset_store());
  nlohmann::json sk2 = minimal_sketch_json_with_underlay_b64(view().asset_store());
  sk1["name"]        = "A";
  sk2["name"]        = "B";
  sk2["isCurrent"]   = false;
  nlohmann::json doc;
  doc["ezyFormat"] = 3;
  doc["shapes"]    = nlohmann::json::array();
  doc["sketches"]  = nlohmann::json::array({sk1, sk2});
  view().load(doc.dump(), false);

  const std::string          manifest  = view().to_json();
  const std::vector<uint8_t> zip_bytes = pack_ezy(manifest, view().asset_store());
  const auto unpacked = unpack_ezy(std::string(reinterpret_cast<const char*>(zip_bytes.data()), zip_bytes.size()));
  ASSERT_TRUE(unpacked);
  EXPECT_EQ(unpacked->assets.size(), 1u);

  const nlohmann::json  out = nlohmann::json::parse(unpacked->manifest_json);
  std::set<std::string> asset_ids;
  for (const nlohmann::json& sk : out["sketches"])
  {
    if (sk.contains("underlay") && sk["underlay"].is_object() && sk["underlay"].contains("asset"))
      asset_ids.insert(sk["underlay"]["asset"].get<std::string>());
  }
  EXPECT_EQ(asset_ids.size(), 1u);
  for (const nlohmann::json& sk : out["sketches"])
  {
    if (sk.contains("underlay") && sk["underlay"].is_object())
      EXPECT_FALSE(sk["underlay"].contains("rgba_b64"));
  }
}

TEST_F(Sketch_test, LegacyUnderlayRgbaB64Load)
{
  view().asset_store().clear();
  const auto sk = Sketch_json::from_json(view(), minimal_sketch_json_with_underlay_b64(view().asset_store()));
  ASSERT_TRUE(sk);
  EXPECT_TRUE(sk->underlay().has_image());
  const nlohmann::json saved = Sketch_json::to_json(*sk, view().asset_store());
  EXPECT_TRUE(saved.contains("underlay"));
  EXPECT_TRUE(saved["underlay"].contains("asset"));
  EXPECT_FALSE(saved["underlay"].contains("rgba_b64"));
}

TEST_F(Sketch_test, UnderlayUndoSnapshotUsesAssetRef)
{
  view().asset_store().clear();
  nlohmann::json doc = nlohmann::json::parse(view().to_json());
  doc["sketches"]    = nlohmann::json::array({minimal_sketch_json_with_underlay_b64(view().asset_store())});
  view().load(doc.dump(), false);

  view().push_undo_snapshot();
  const std::string snap = view().to_json();
  EXPECT_EQ(snap.find("rgba_b64"), std::string::npos);
  EXPECT_NE(snap.find("\"asset\""), std::string::npos);
}

namespace
{

nlohmann::json minimal_sketch_json_with_underlay_b64(Ezy_asset_store& store)
{
  std::vector<uint8_t> rgba(16);
  for (int i = 0; i < 4; ++i)
  {
    rgba[static_cast<std::size_t>(i) * 4u + 0] = 255;
    rgba[static_cast<std::size_t>(i) * 4u + 1] = 0;
    rgba[static_cast<std::size_t>(i) * 4u + 2] = 0;
    rgba[static_cast<std::size_t>(i) * 4u + 3] = 255;
  }

  nlohmann::json j;
  j["isCurrent"] = true;
  j["name"]      = "UnderlaySketch";
  j["plane"]     = nlohmann::json::object({{"origin", {{"x", 0.0}, {"y", 0.0}, {"z", 0.0}}},
                                           {"normal", {{"x", 0.0}, {"y", 0.0}, {"z", 1.0}}},
                                           {"xAxis", {{"x", 1.0}, {"y", 0.0}, {"z", 0.0}}}});
  j["edges"]     = nlohmann::json::array();
  j["nodes"]     = nlohmann::json::array();
  j["arc_edges"] = nlohmann::json::array();
  j["underlay"] =
      nlohmann::json::object({{"rgba_b64", ezy_base64_encode(rgba)}, {"w", 2}, {"h", 2}, {"opacity", 0.88}, {"visible", true}});
  (void)store;
  return j;
}

} // namespace
