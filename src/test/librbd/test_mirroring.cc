// -*- mode:C; tab-width:8; c-basic-offset:2; indent-tabs-mode:t -*-
// vim: ts=8 sw=2 smarttab
/*
 * Ceph - scalable distributed file system
 *
 * Copyright (C) 2016 SUSE LINUX GmbH
 *
 * This is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License version 2.1, as published by the Free Software
 * Foundation.  See file COPYING.
 *
 */
#include "test/librbd/test_fixture.h"
#include "test/librbd/test_support.h"
#include "librbd/AioCompletion.h"
#include "librbd/AioImageRequest.h"
#include "librbd/AioImageRequestWQ.h"
#include "librbd/ExclusiveLock.h"
#include "librbd/ImageState.h"
#include "librbd/ImageWatcher.h"
#include "librbd/internal.h"
#include "librbd/ObjectMap.h"
#include "librbd/Operations.h"
#include <boost/scope_exit.hpp>
#include <boost/assign/list_of.hpp>
#include <utility>
#include <vector>

void register_test_mirroring() {
}

class TestMirroring : public TestFixture {
public:

  TestMirroring() {}


  virtual void TearDown() {
    unlock_image();

    TestFixture::TearDown();
  }

  virtual void SetUp() {
    ASSERT_EQ(0, _rados.ioctx_create(_pool_name.c_str(), m_ioctx));
  }

  std::string image_name = "mirrorimg1";

  void check_mirror_image_enable(rbd_mirror_mode_t mirror_mode,
                                 uint64_t features,
                                 int expected_r,
                                 rbd_mirror_image_state_t mirror_state) {

    ASSERT_EQ(0, m_rbd.mirror_mode_set(m_ioctx, RBD_MIRROR_MODE_DISABLED));

    int order = 20;
    ASSERT_EQ(0, m_rbd.create2(m_ioctx, image_name.c_str(), 4096, features, &order));
    librbd::Image image;
    ASSERT_EQ(0, m_rbd.open(m_ioctx, image, image_name.c_str()));

    ASSERT_EQ(0, m_rbd.mirror_mode_set(m_ioctx, mirror_mode));

    ASSERT_EQ(expected_r, image.mirror_image_enable());

    librbd::mirror_image_info_t mirror_image;
    ASSERT_EQ(0, image.mirror_image_get_info(&mirror_image, sizeof(mirror_image)));
    ASSERT_EQ(mirror_state, mirror_image.state);

    ASSERT_EQ(0, image.close());
    ASSERT_EQ(0, m_rbd.remove(m_ioctx, image_name.c_str()));
    ASSERT_EQ(0, m_rbd.mirror_mode_set(m_ioctx, RBD_MIRROR_MODE_DISABLED));
  }

  void check_mirror_image_disable(rbd_mirror_mode_t mirror_mode,
                                  uint64_t features,
                                  int expected_r,
                                  rbd_mirror_image_state_t mirror_state) {

    ASSERT_EQ(0, m_rbd.mirror_mode_set(m_ioctx, RBD_MIRROR_MODE_POOL));

    int order = 20;
    ASSERT_EQ(0, m_rbd.create2(m_ioctx, image_name.c_str(), 4096, features, &order));
    librbd::Image image;
    ASSERT_EQ(0, m_rbd.open(m_ioctx, image, image_name.c_str()));

    ASSERT_EQ(0, m_rbd.mirror_mode_set(m_ioctx, mirror_mode));

    ASSERT_EQ(expected_r, image.mirror_image_disable(false));

    librbd::mirror_image_info_t mirror_image;
    ASSERT_EQ(0, image.mirror_image_get_info(&mirror_image, sizeof(mirror_image)));
    ASSERT_EQ(mirror_state, mirror_image.state);

    ASSERT_EQ(0, image.close());
    ASSERT_EQ(0, m_rbd.remove(m_ioctx, image_name.c_str()));
    ASSERT_EQ(0, m_rbd.mirror_mode_set(m_ioctx, RBD_MIRROR_MODE_DISABLED));
  }

  void check_mirroring_on_create(uint64_t features,
                                 rbd_mirror_mode_t mirror_mode,
                                 rbd_mirror_image_state_t mirror_state) {

    ASSERT_EQ(0, m_rbd.mirror_mode_set(m_ioctx, mirror_mode));

    int order = 20;
    ASSERT_EQ(0, m_rbd.create2(m_ioctx, image_name.c_str(), 4096, features, &order));
    librbd::Image image;
    ASSERT_EQ(0, m_rbd.open(m_ioctx, image, image_name.c_str()));

    librbd::mirror_image_info_t mirror_image;
    ASSERT_EQ(0, image.mirror_image_get_info(&mirror_image, sizeof(mirror_image)));
    ASSERT_EQ(mirror_state, mirror_image.state);

    ASSERT_EQ(0, image.close());
    ASSERT_EQ(0, m_rbd.remove(m_ioctx, image_name.c_str()));
    ASSERT_EQ(0, m_rbd.mirror_mode_set(m_ioctx, RBD_MIRROR_MODE_DISABLED));
  }

  void check_mirroring_on_update_features(uint64_t init_features,
                                 bool enable, bool enable_mirroring,
                                 uint64_t features, int expected_r,
                                 rbd_mirror_mode_t mirror_mode,
                                 rbd_mirror_image_state_t mirror_state) {

    ASSERT_EQ(0, m_rbd.mirror_mode_set(m_ioctx, mirror_mode));

    int order = 20;
    ASSERT_EQ(0, m_rbd.create2(m_ioctx, image_name.c_str(), 4096, init_features, &order));
    librbd::Image image;
    ASSERT_EQ(0, m_rbd.open(m_ioctx, image, image_name.c_str()));

    if (enable_mirroring) {
      ASSERT_EQ(0, image.mirror_image_enable());
    }

    ASSERT_EQ(expected_r, image.update_features(features, enable));

    librbd::mirror_image_info_t mirror_image;
    ASSERT_EQ(0, image.mirror_image_get_info(&mirror_image, sizeof(mirror_image)));
    ASSERT_EQ(mirror_state, mirror_image.state);

    ASSERT_EQ(0, image.close());
    ASSERT_EQ(0, m_rbd.remove(m_ioctx, image_name.c_str()));
    ASSERT_EQ(0, m_rbd.mirror_mode_set(m_ioctx, RBD_MIRROR_MODE_DISABLED));
  }

  void setup_images_with_mirror_mode(rbd_mirror_mode_t mirror_mode,
                                        std::vector<uint64_t>& features_vec) {

    ASSERT_EQ(0, m_rbd.mirror_mode_set(m_ioctx, mirror_mode));

    int id = 1;
    int order = 20;
    for (const auto& features : features_vec) {
      std::stringstream img_name("img_");
      img_name << id++;
      std::string img_name_str = img_name.str();
      ASSERT_EQ(0, m_rbd.create2(m_ioctx, img_name_str.c_str(), 2048, features, &order));
    }
  }

  void check_mirroring_on_mirror_mode_set(rbd_mirror_mode_t mirror_mode,
                            std::vector<rbd_mirror_image_state_t>& states_vec) {

    ASSERT_EQ(0, m_rbd.mirror_mode_set(m_ioctx, mirror_mode));

    std::vector< std::tuple<std::string, rbd_mirror_image_state_t> > images;
    int id = 1;
    for (const auto& mirror_state : states_vec) {
      std::stringstream img_name("img_");
      img_name << id++;
      std::string img_name_str = img_name.str();
      librbd::Image image;
      ASSERT_EQ(0, m_rbd.open(m_ioctx, image, img_name_str.c_str()));
      images.push_back(std::make_tuple(img_name_str, mirror_state));

      librbd::mirror_image_info_t mirror_image;
      ASSERT_EQ(0, image.mirror_image_get_info(&mirror_image, sizeof(mirror_image)));
      ASSERT_EQ(mirror_state, mirror_image.state);

      ASSERT_EQ(0, image.close());
      ASSERT_EQ(0, m_rbd.remove(m_ioctx, img_name_str.c_str()));
    }
  }

  void check_remove_image(rbd_mirror_mode_t mirror_mode, uint64_t features,
                          bool enable_mirroring) {

    ASSERT_EQ(0, m_rbd.mirror_mode_set(m_ioctx, mirror_mode));

    int order = 20;
    ASSERT_EQ(0, m_rbd.create2(m_ioctx, image_name.c_str(), 4096, features,
              &order));
    librbd::Image image;
    ASSERT_EQ(0, m_rbd.open(m_ioctx, image, image_name.c_str()));

    if (enable_mirroring) {
      ASSERT_EQ(0, image.mirror_image_enable());
    }

    image.close();
    ASSERT_EQ(0, m_rbd.remove(m_ioctx, image_name.c_str()));
    ASSERT_EQ(0, m_rbd.mirror_mode_set(m_ioctx, RBD_MIRROR_MODE_DISABLED));
  }

};

TEST_F(TestMirroring, EnableImageMirror_In_MirrorModeImage) {
  uint64_t features = 0;
  features |= RBD_FEATURE_OBJECT_MAP;
  features |= RBD_FEATURE_EXCLUSIVE_LOCK;
  features |= RBD_FEATURE_JOURNALING;
  check_mirror_image_enable(RBD_MIRROR_MODE_IMAGE, features, 0,
      RBD_MIRROR_IMAGE_ENABLED);
}

TEST_F(TestMirroring, EnableImageMirror_In_MirrorModePool) {
  uint64_t features = 0;
  features |= RBD_FEATURE_OBJECT_MAP;
  features |= RBD_FEATURE_EXCLUSIVE_LOCK;
  features |= RBD_FEATURE_JOURNALING;
  check_mirror_image_enable(RBD_MIRROR_MODE_POOL, features, -EINVAL,
      RBD_MIRROR_IMAGE_ENABLED);
}

TEST_F(TestMirroring, EnableImageMirror_In_MirrorModeDisabled) {
  uint64_t features = 0;
  features |= RBD_FEATURE_OBJECT_MAP;
  features |= RBD_FEATURE_EXCLUSIVE_LOCK;
  features |= RBD_FEATURE_JOURNALING;
  check_mirror_image_enable(RBD_MIRROR_MODE_DISABLED, features, -EINVAL,
      RBD_MIRROR_IMAGE_DISABLED);
}

TEST_F(TestMirroring, DisableImageMirror_In_MirrorModeImage) {
  uint64_t features = 0;
  features |= RBD_FEATURE_OBJECT_MAP;
  features |= RBD_FEATURE_EXCLUSIVE_LOCK;
  features |= RBD_FEATURE_JOURNALING;
  check_mirror_image_disable(RBD_MIRROR_MODE_IMAGE, features, 0,
      RBD_MIRROR_IMAGE_DISABLED);
}

TEST_F(TestMirroring, DisableImageMirror_In_MirrorModePool) {
  uint64_t features = 0;
  features |= RBD_FEATURE_OBJECT_MAP;
  features |= RBD_FEATURE_EXCLUSIVE_LOCK;
  features |= RBD_FEATURE_JOURNALING;
  check_mirror_image_disable(RBD_MIRROR_MODE_POOL, features, -EINVAL,
      RBD_MIRROR_IMAGE_ENABLED);
}

TEST_F(TestMirroring, DisableImageMirror_In_MirrorModeDisabled) {
  uint64_t features = 0;
  features |= RBD_FEATURE_OBJECT_MAP;
  features |= RBD_FEATURE_EXCLUSIVE_LOCK;
  features |= RBD_FEATURE_JOURNALING;
  check_mirror_image_disable(RBD_MIRROR_MODE_DISABLED, features, -EINVAL,
      RBD_MIRROR_IMAGE_DISABLED);
}

TEST_F(TestMirroring, EnableImageMirror_WithoutJournaling) {
  uint64_t features = 0;
  features |= RBD_FEATURE_OBJECT_MAP;
  features |= RBD_FEATURE_EXCLUSIVE_LOCK;
  check_mirror_image_enable(RBD_MIRROR_MODE_DISABLED, features, -EINVAL,
      RBD_MIRROR_IMAGE_DISABLED);
}

TEST_F(TestMirroring, CreateImage_In_MirrorModeDisabled) {
  uint64_t features = 0;
  features |= RBD_FEATURE_OBJECT_MAP;
  features |= RBD_FEATURE_EXCLUSIVE_LOCK;
  features |= RBD_FEATURE_JOURNALING;
  check_mirroring_on_create(features, RBD_MIRROR_MODE_DISABLED,
                            RBD_MIRROR_IMAGE_DISABLED);
}

TEST_F(TestMirroring, CreateImage_In_MirrorModeImage) {
  uint64_t features = 0;
  features |= RBD_FEATURE_OBJECT_MAP;
  features |= RBD_FEATURE_EXCLUSIVE_LOCK;
  features |= RBD_FEATURE_JOURNALING;
  check_mirroring_on_create(features, RBD_MIRROR_MODE_IMAGE,
                            RBD_MIRROR_IMAGE_DISABLED);
}

TEST_F(TestMirroring, CreateImage_In_MirrorModePool) {
  uint64_t features = 0;
  features |= RBD_FEATURE_OBJECT_MAP;
  features |= RBD_FEATURE_EXCLUSIVE_LOCK;
  features |= RBD_FEATURE_JOURNALING;
  check_mirroring_on_create(features, RBD_MIRROR_MODE_POOL,
                            RBD_MIRROR_IMAGE_ENABLED);
}

TEST_F(TestMirroring, CreateImage_In_MirrorModePool_WithoutJournaling) {
  uint64_t features = 0;
  features |= RBD_FEATURE_OBJECT_MAP;
  features |= RBD_FEATURE_EXCLUSIVE_LOCK;
  check_mirroring_on_create(features, RBD_MIRROR_MODE_POOL,
                            RBD_MIRROR_IMAGE_DISABLED);
}

TEST_F(TestMirroring, CreateImage_In_MirrorModeImage_WithoutJournaling) {
  uint64_t features = 0;
  features |= RBD_FEATURE_OBJECT_MAP;
  features |= RBD_FEATURE_EXCLUSIVE_LOCK;
  check_mirroring_on_create(features, RBD_MIRROR_MODE_IMAGE,
                            RBD_MIRROR_IMAGE_DISABLED);
}

TEST_F(TestMirroring, EnableJournaling_In_MirrorModeDisabled) {
  uint64_t init_features = 0;
  init_features |= RBD_FEATURE_OBJECT_MAP;
  init_features |= RBD_FEATURE_EXCLUSIVE_LOCK;
  uint64_t features = init_features | RBD_FEATURE_JOURNALING;
  check_mirroring_on_update_features(init_features, true, false, features, 0,
                      RBD_MIRROR_MODE_DISABLED, RBD_MIRROR_IMAGE_DISABLED);
}

TEST_F(TestMirroring, EnableJournaling_In_MirrorModeImage) {
  uint64_t init_features = 0;
  init_features |= RBD_FEATURE_OBJECT_MAP;
  init_features |= RBD_FEATURE_EXCLUSIVE_LOCK;
  uint64_t features = init_features | RBD_FEATURE_JOURNALING;
  check_mirroring_on_update_features(init_features, true, false, features, 0,
                      RBD_MIRROR_MODE_IMAGE, RBD_MIRROR_IMAGE_DISABLED);
}

TEST_F(TestMirroring, EnableJournaling_In_MirrorModePool) {
  uint64_t init_features = 0;
  init_features |= RBD_FEATURE_OBJECT_MAP;
  init_features |= RBD_FEATURE_EXCLUSIVE_LOCK;
  uint64_t features = init_features | RBD_FEATURE_JOURNALING;
  check_mirroring_on_update_features(init_features, true, false, features, 0,
                      RBD_MIRROR_MODE_POOL, RBD_MIRROR_IMAGE_ENABLED);
}

TEST_F(TestMirroring, DisableJournaling_In_MirrorModePool) {
  uint64_t init_features = 0;
  init_features |= RBD_FEATURE_OBJECT_MAP;
  init_features |= RBD_FEATURE_EXCLUSIVE_LOCK;
  init_features |= RBD_FEATURE_JOURNALING;
  uint64_t features = RBD_FEATURE_JOURNALING;
  check_mirroring_on_update_features(init_features, false, false, features, 0,
                      RBD_MIRROR_MODE_POOL, RBD_MIRROR_IMAGE_DISABLED);
}

TEST_F(TestMirroring, DisableJournaling_In_MirrorModeImage) {
  uint64_t init_features = 0;
  init_features |= RBD_FEATURE_OBJECT_MAP;
  init_features |= RBD_FEATURE_EXCLUSIVE_LOCK;
  init_features |= RBD_FEATURE_JOURNALING;
  uint64_t features = RBD_FEATURE_JOURNALING;
  check_mirroring_on_update_features(init_features, false, true, features, -EINVAL,
                      RBD_MIRROR_MODE_IMAGE, RBD_MIRROR_IMAGE_ENABLED);
}

TEST_F(TestMirroring, MirrorModeSet_DisabledMode_To_PoolMode) {
  std::vector<uint64_t> features_vec;
  features_vec.push_back(RBD_FEATURE_EXCLUSIVE_LOCK);
  features_vec.push_back(RBD_FEATURE_EXCLUSIVE_LOCK | RBD_FEATURE_JOURNALING);

  setup_images_with_mirror_mode(RBD_MIRROR_MODE_DISABLED, features_vec);

  std::vector<rbd_mirror_image_state_t> states_vec;
  states_vec.push_back(RBD_MIRROR_IMAGE_DISABLED);
  states_vec.push_back(RBD_MIRROR_IMAGE_ENABLED);
  check_mirroring_on_mirror_mode_set(RBD_MIRROR_MODE_POOL, states_vec);
}

TEST_F(TestMirroring, MirrorModeSet_PoolMode_To_DisabledMode) {
  std::vector<uint64_t> features_vec;
  features_vec.push_back(RBD_FEATURE_EXCLUSIVE_LOCK);
  features_vec.push_back(RBD_FEATURE_EXCLUSIVE_LOCK | RBD_FEATURE_JOURNALING);

  setup_images_with_mirror_mode(RBD_MIRROR_MODE_POOL, features_vec);

  std::vector<rbd_mirror_image_state_t> states_vec;
  states_vec.push_back(RBD_MIRROR_IMAGE_DISABLED);
  states_vec.push_back(RBD_MIRROR_IMAGE_DISABLED);
  check_mirroring_on_mirror_mode_set(RBD_MIRROR_MODE_DISABLED, states_vec);
}

TEST_F(TestMirroring, MirrorModeSet_DisabledMode_To_ImageMode) {
  std::vector<uint64_t> features_vec;
  features_vec.push_back(RBD_FEATURE_EXCLUSIVE_LOCK);
  features_vec.push_back(RBD_FEATURE_EXCLUSIVE_LOCK | RBD_FEATURE_JOURNALING);

  setup_images_with_mirror_mode(RBD_MIRROR_MODE_DISABLED, features_vec);

  std::vector<rbd_mirror_image_state_t> states_vec;
  states_vec.push_back(RBD_MIRROR_IMAGE_DISABLED);
  states_vec.push_back(RBD_MIRROR_IMAGE_DISABLED);
  check_mirroring_on_mirror_mode_set(RBD_MIRROR_MODE_IMAGE, states_vec);
}


TEST_F(TestMirroring, MirrorModeSet_PoolMode_To_ImageMode) {
  std::vector<uint64_t> features_vec;
  features_vec.push_back(RBD_FEATURE_EXCLUSIVE_LOCK);
  features_vec.push_back(RBD_FEATURE_EXCLUSIVE_LOCK | RBD_FEATURE_JOURNALING);

  setup_images_with_mirror_mode(RBD_MIRROR_MODE_POOL, features_vec);

  std::vector<rbd_mirror_image_state_t> states_vec;
  states_vec.push_back(RBD_MIRROR_IMAGE_DISABLED);
  states_vec.push_back(RBD_MIRROR_IMAGE_ENABLED);
  check_mirroring_on_mirror_mode_set(RBD_MIRROR_MODE_IMAGE, states_vec);
}

TEST_F(TestMirroring, MirrorModeSet_ImageMode_To_PoolMode) {
  std::vector<uint64_t> features_vec;
  features_vec.push_back(RBD_FEATURE_EXCLUSIVE_LOCK);
  features_vec.push_back(RBD_FEATURE_EXCLUSIVE_LOCK | RBD_FEATURE_JOURNALING);

  setup_images_with_mirror_mode(RBD_MIRROR_MODE_IMAGE, features_vec);

  std::vector<rbd_mirror_image_state_t> states_vec;
  states_vec.push_back(RBD_MIRROR_IMAGE_DISABLED);
  states_vec.push_back(RBD_MIRROR_IMAGE_ENABLED);
  check_mirroring_on_mirror_mode_set(RBD_MIRROR_MODE_POOL, states_vec);
}

TEST_F(TestMirroring, MirrorModeSet_ImageMode_To_DisabledMode) {
  std::vector<uint64_t> features_vec;
  features_vec.push_back(RBD_FEATURE_EXCLUSIVE_LOCK);
  features_vec.push_back(RBD_FEATURE_EXCLUSIVE_LOCK | RBD_FEATURE_JOURNALING);

  setup_images_with_mirror_mode(RBD_MIRROR_MODE_POOL, features_vec);

  ASSERT_EQ(0, m_rbd.mirror_mode_set(m_ioctx, RBD_MIRROR_MODE_IMAGE));
  ASSERT_EQ(-EINVAL, m_rbd.mirror_mode_set(m_ioctx, RBD_MIRROR_MODE_DISABLED));
  ASSERT_EQ(0, m_rbd.mirror_mode_set(m_ioctx, RBD_MIRROR_MODE_POOL));

  std::vector<rbd_mirror_image_state_t> states_vec;
  states_vec.push_back(RBD_MIRROR_IMAGE_DISABLED);
  states_vec.push_back(RBD_MIRROR_IMAGE_DISABLED);
  check_mirroring_on_mirror_mode_set(RBD_MIRROR_MODE_DISABLED, states_vec);
}

TEST_F(TestMirroring, RemoveImage_With_MirrorImageEnabled) {
  check_remove_image(RBD_MIRROR_MODE_IMAGE,
                     RBD_FEATURE_EXCLUSIVE_LOCK | RBD_FEATURE_JOURNALING,
                     true);
}

TEST_F(TestMirroring, RemoveImage_With_MirrorImageDisabled) {
  check_remove_image(RBD_MIRROR_MODE_IMAGE,
                     RBD_FEATURE_EXCLUSIVE_LOCK | RBD_FEATURE_JOURNALING,
                     false);
}

TEST_F(TestMirroring, RemoveImage_With_ImageWithoutJournal) {
  check_remove_image(RBD_MIRROR_MODE_IMAGE,
                     RBD_FEATURE_EXCLUSIVE_LOCK,
                     false);
}

