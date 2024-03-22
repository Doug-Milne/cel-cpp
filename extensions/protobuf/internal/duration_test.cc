// Copyright 2023 Google LLC
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     https://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "extensions/protobuf/internal/duration.h"

#include <memory>

#include "google/protobuf/duration.pb.h"
#include "google/protobuf/descriptor.pb.h"
#include "absl/memory/memory.h"
#include "absl/time/time.h"
#include "extensions/protobuf/internal/duration_lite.h"
#include "internal/testing.h"
#include "google/protobuf/descriptor.h"
#include "google/protobuf/descriptor_database.h"
#include "google/protobuf/dynamic_message.h"

namespace cel::extensions::protobuf_internal {
namespace {

using testing::Eq;
using cel::internal::IsOkAndHolds;

TEST(Duration, GeneratedFromProto) {
  EXPECT_THAT(UnwrapGeneratedDurationProto(google::protobuf::Duration()),
              IsOkAndHolds(Eq(absl::ZeroDuration())));
}

TEST(Duration, CustomFromProto) {
  google::protobuf::SimpleDescriptorDatabase database;
  {
    google::protobuf::FileDescriptorProto fd;
    google::protobuf::Duration::descriptor()->file()->CopyTo(&fd);
    ASSERT_TRUE(database.Add(fd));
  }
  google::protobuf::DescriptorPool pool(&database);
  pool.AllowUnknownDependencies();
  google::protobuf::DynamicMessageFactory factory(&pool);
  factory.SetDelegateToGeneratedFactory(false);
  EXPECT_THAT(UnwrapDynamicDurationProto(*factory.GetPrototype(
                  pool.FindMessageTypeByName("google.protobuf.Duration"))),
              IsOkAndHolds(Eq(absl::ZeroDuration())));
}

TEST(Duration, GeneratedToProto) {
  google::protobuf::Duration proto;
  ASSERT_OK(WrapGeneratedDurationProto(absl::Seconds(1) + absl::Nanoseconds(2),
                                       proto));
  EXPECT_EQ(proto.seconds(), 1);
  EXPECT_EQ(proto.nanos(), 2);
}

TEST(Duration, CustomToProto) {
  google::protobuf::SimpleDescriptorDatabase database;
  {
    google::protobuf::FileDescriptorProto fd;
    google::protobuf::Duration::descriptor()->file()->CopyTo(&fd);
    ASSERT_TRUE(database.Add(fd));
  }
  google::protobuf::DescriptorPool pool(&database);
  pool.AllowUnknownDependencies();
  google::protobuf::DynamicMessageFactory factory(&pool);
  factory.SetDelegateToGeneratedFactory(false);
  std::unique_ptr<google::protobuf::Message> proto = absl::WrapUnique(
      factory
          .GetPrototype(pool.FindMessageTypeByName("google.protobuf.Duration"))
          ->New());
  const auto* descriptor = proto->GetDescriptor();
  const auto* reflection = proto->GetReflection();
  const auto* seconds_field = descriptor->FindFieldByName("seconds");
  ASSERT_NE(seconds_field, nullptr);
  const auto* nanos_field = descriptor->FindFieldByName("nanos");
  ASSERT_NE(nanos_field, nullptr);

  ASSERT_OK(WrapDynamicDurationProto(absl::Seconds(1) + absl::Nanoseconds(2),
                                     *proto));

  EXPECT_EQ(reflection->GetInt64(*proto, seconds_field), 1);
  EXPECT_EQ(reflection->GetInt32(*proto, nanos_field), 2);
}

}  // namespace
}  // namespace cel::extensions::protobuf_internal
