//*****************************************************************************
// Copyright 2017-2019 Intel Corporation
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//*****************************************************************************

#include "gtest/gtest.h"

#include "ngraph/ngraph.hpp"
#include "ngraph/op/experimental/layers/detection_output.hpp"
#include "ngraph/op/experimental/layers/interpolate.hpp"
#include "ngraph/op/experimental/layers/prior_box.hpp"
#include "ngraph/op/experimental/layers/prior_box_clustered.hpp"
#include "ngraph/op/experimental/layers/proposal.hpp"
#include "ngraph/op/experimental/layers/region_yolo.hpp"
#include "ngraph/op/experimental/layers/reorg_yolo.hpp"

#include <memory>
using namespace std;
using namespace ngraph;

TEST(type_prop_layers, detection_output)
{
    auto box_logits = make_shared<op::Parameter>(element::f32, Shape{4, 1, 5, 5});
    auto class_preds = make_shared<op::Parameter>(element::f32, Shape{2, 1, 4, 5});
    auto proposals = make_shared<op::Parameter>(element::f32, Shape{2, 1, 4, 5});
    auto aux_class_preds = make_shared<op::Parameter>(element::f32, Shape{2, 1, 4, 5});
    auto aux_box_preds = make_shared<op::Parameter>(element::f32, Shape{2, 1, 4, 5});
    op::DetectionOutputAttrs attrs;
    attrs.boxes_top_k = {200};
    auto op = make_shared<op::DetectionOutput>(
        box_logits, class_preds, proposals, aux_class_preds, aux_box_preds, attrs);
    ASSERT_EQ(op->get_shape(), (Shape{1, 1, 800, 7}));
}

TEST(type_prop_layers, interpolate)
{
    auto image = make_shared<op::Parameter>(element::f32, Shape{2, 2, 33, 65});

    op::InterpolateAttrs attrs1;
    attrs1.height = 257;
    attrs1.width = 513;
    auto op1 = make_shared<op::Interpolate>(image, attrs1);
    ASSERT_EQ(op1->get_shape(), (Shape{2, 2, 257, 513}));

    op::InterpolateAttrs attrs2;
    attrs2.scale_factor = 2;
    attrs2.width = 513;
    auto op2 = make_shared<op::Interpolate>(image, attrs2);
    ASSERT_EQ(op2->get_shape(), (Shape{2, 2, 66, 513}));

    op::InterpolateAttrs attrs3;
    attrs3.scale_factor = 2;
    attrs3.height = 257;
    auto op3 = make_shared<op::Interpolate>(image, attrs3);
    ASSERT_EQ(op3->get_shape(), (Shape{2, 2, 257, 130}));
}

TEST(type_prop_layers, dyn_interpolate)
{
    auto image = make_shared<op::Parameter>(element::f32, Shape{2, 2, 33, 65});
    auto output_shape = op::Constant::create<int64_t>(element::i64, Shape{4}, {4, 2, 15, 30});

    op::InterpolateAttrs attrs;
    attrs.height = 257;
    attrs.width = 513;
    auto op = make_shared<op::DynInterpolate>(image, output_shape, attrs);
    ASSERT_EQ(op->get_shape(), (Shape{4, 2, 15, 30}));
}

TEST(type_prop_layers, prior_box1)
{
    auto layer_shape = op::Constant::create<int64_t>(element::i64, Shape{2}, {32, 32});
    auto image_shape = op::Constant::create<int64_t>(element::i64, Shape{2}, {300, 300});
    auto pb = make_shared<op::PriorBox>(layer_shape,
                                        image_shape,
                                        std::vector<float>{2.0f, 3.0f},
                                        std::vector<float>{},
                                        std::vector<float>{1.0f, 2.0f, 0.5f},
                                        false,
                                        false,
                                        1.0f,
                                        0.5f,
                                        std::vector<float>{1.0f, 0.0f, 0.0f, 2.0f},
                                        false);
    ASSERT_EQ(pb->get_shape(), (Shape{2, 16384}));
}

TEST(type_prop_layers, prior_box2)
{
    auto layer_shape = op::Constant::create<int64_t>(element::i64, Shape{2}, {32, 32});
    auto image_shape = op::Constant::create<int64_t>(element::i64, Shape{2}, {300, 300});
    auto pb = make_shared<op::PriorBox>(layer_shape,
                                        image_shape,
                                        std::vector<float>{2.0f, 3.0f},
                                        std::vector<float>{},
                                        std::vector<float>{1.0f, 2.0f, 0.5f},
                                        false,
                                        true,
                                        1.0f,
                                        0.5f,
                                        std::vector<float>{1.0f, 0.0f, 0.0f, 2.0f},
                                        false);
    ASSERT_EQ(pb->get_shape(), (Shape{2, 28672}));
}

TEST(type_prop_layers, prior_box3)
{
    auto layer_shape = op::Constant::create<int64_t>(element::i64, Shape{2}, {1, 1});
    auto image_shape = op::Constant::create<int64_t>(element::i64, Shape{2}, {300, 300});
    auto pb = make_shared<op::PriorBox>(layer_shape,
                                        image_shape,
                                        std::vector<float>{256.0f},
                                        std::vector<float>{315.0f},
                                        std::vector<float>{2.0f},
                                        false,
                                        true,
                                        1.0f,
                                        0.5f,
                                        std::vector<float>{1.0f, 0.0f, 0.0f, 2.0f},
                                        true);
    ASSERT_EQ(pb->get_shape(), (Shape{2, 16}));
}

TEST(type_prop_layers, prior_box_clustered)
{
    auto layer_shape = op::Constant::create<int64_t>(element::i64, Shape{2}, {19, 19});
    auto image_shape = op::Constant::create<int64_t>(element::i64, Shape{2}, {300, 300});
    auto pbc = make_shared<op::PriorBoxClustered>(layer_shape,
                                                  image_shape,
                                                  3,
                                                  std::vector<float>{4.0f, 2.0f, 3.2f},
                                                  std::vector<float>{1.0f, 2.0f, 1.1f},
                                                  false,
                                                  1.0f,
                                                  2.0f,
                                                  0.0f,
                                                  std::vector<float>{1.0f, 0.0f, 0.0f, 2.0f});
    // Output shape - 4 * 19 * 19 * 3 (num_priors)
    ASSERT_EQ(pbc->get_shape(), (Shape{2, 4332}));
}

TEST(type_prop_layers, proposal)
{
    auto class_probs = make_shared<op::Parameter>(element::f32, Shape{1, 12, 34, 62});
    auto class_logits = make_shared<op::Parameter>(element::f32, Shape{1, 24, 34, 62});
    auto image_shape = op::Constant::create<int64_t>(element::i64, Shape{2}, {1, 6});
    auto op = make_shared<op::Proposal>(class_probs,
                                        class_logits,
                                        image_shape,
                                        1,
                                        20,
                                        200,
                                        0.0f,
                                        1,
                                        1,
                                        std::vector<float>{},
                                        std::vector<float>{},
                                        false,
                                        false,
                                        false,
                                        0.1f,
                                        0.1f,
                                        std::string{""});
    ASSERT_EQ(op->get_shape(), (Shape{200, 5}));
}

TEST(type_prop_layers, region_yolo1)
{
    auto inputs = make_shared<op::Parameter>(element::f32, Shape{1, 125, 13, 13});
    auto op = make_shared<op::RegionYolo>(inputs, 0, 0, 0, true, std::vector<int64_t>{}, 0, 1);
    ASSERT_EQ(op->get_shape(), (Shape{1 * 125, 13, 13}));
}

TEST(type_prop_layers, region_yolo2)
{
    auto inputs = make_shared<op::Parameter>(element::f32, Shape{1, 125, 13, 13});
    auto op = make_shared<op::RegionYolo>(inputs, 0, 0, 0, true, std::vector<int64_t>{}, 0, 2);
    ASSERT_EQ(op->get_shape(), (Shape{1 * 125 * 13, 13}));
}

TEST(type_prop_layers, region_yolo3)
{
    auto inputs = make_shared<op::Parameter>(element::f32, Shape{1, 125, 13, 13});
    auto op =
        make_shared<op::RegionYolo>(inputs, 4, 80, 1, false, std::vector<int64_t>{6, 7, 8}, 0, -1);
    ASSERT_EQ(op->get_shape(), (Shape{1, (80 + 4 + 1) * 3, 13, 13}));
}

TEST(type_prop_layers, reorg_yolo)
{
    auto inputs = make_shared<op::Parameter>(element::f32, Shape{2, 24, 34, 62});
    auto op = make_shared<op::ReorgYolo>(inputs, Strides{2});
    ASSERT_EQ(op->get_shape(), (Shape{2, 96, 17, 31}));
}
