#include "envoy/config/filter/network/thrift_proxy/v2alpha1/route.pb.h"
#include "envoy/config/filter/network/thrift_proxy/v2alpha1/route.pb.validate.h"

#include "extensions/filters/network/thrift_proxy/router/config.h"
#include "extensions/filters/network/thrift_proxy/router/router_impl.h"

#include "test/extensions/filters/network/thrift_proxy/utility.h"
#include "test/test_common/utility.h"

#include "gtest/gtest.h"

using testing::_;
using testing::Test;

namespace Envoy {
namespace Extensions {
namespace NetworkFilters {
namespace ThriftProxy {
namespace Router {

namespace {

envoy::config::filter::network::thrift_proxy::v2alpha1::RouteConfiguration
parseRouteConfigurationFromV2Yaml(const std::string& yaml) {
  envoy::config::filter::network::thrift_proxy::v2alpha1::RouteConfiguration route_config;
  MessageUtil::loadFromYaml(yaml, route_config);
  MessageUtil::validate(route_config);
  return route_config;
}

TEST(RouteMatcherTest, RouteByMethodNameWithNoInversion) {
  const std::string yaml = R"EOF(
name: config
routes:
  - match:
      method_name: "method1"
    route:
      cluster: "cluster1"
  - match:
      method_name: "method2"
    route:
      cluster: "cluster2"
)EOF";

  envoy::config::filter::network::thrift_proxy::v2alpha1::RouteConfiguration config =
      parseRouteConfigurationFromV2Yaml(yaml);

  RouteMatcher matcher(config);
  MessageMetadata metadata;
  EXPECT_EQ(nullptr, matcher.route(metadata, 0));
  metadata.setMethodName("unknown");
  EXPECT_EQ(nullptr, matcher.route(metadata, 0));
  metadata.setMethodName("METHOD1");
  EXPECT_EQ(nullptr, matcher.route(metadata, 0));

  metadata.setMethodName("method1");
  RouteConstSharedPtr route = matcher.route(metadata, 0);
  EXPECT_NE(nullptr, route);
  EXPECT_EQ("cluster1", route->routeEntry()->clusterName());

  metadata.setMethodName("method2");
  RouteConstSharedPtr route2 = matcher.route(metadata, 0);
  EXPECT_NE(nullptr, route2);
  EXPECT_EQ("cluster2", route2->routeEntry()->clusterName());
}

TEST(RouteMatcherTest, RouteByMethodNameWithInversion) {
  const std::string yaml = R"EOF(
name: config
routes:
  - match:
      method_name: "method1"
    route:
      cluster: "cluster1"
  - match:
      method_name: "method2"
      invert: true
    route:
      cluster: "cluster2"
)EOF";

  envoy::config::filter::network::thrift_proxy::v2alpha1::RouteConfiguration config =
      parseRouteConfigurationFromV2Yaml(yaml);

  RouteMatcher matcher(config);
  MessageMetadata metadata;
  RouteConstSharedPtr route = matcher.route(metadata, 0);
  EXPECT_NE(nullptr, route);
  EXPECT_EQ("cluster2", route->routeEntry()->clusterName());

  metadata.setMethodName("unknown");
  route = matcher.route(metadata, 0);
  EXPECT_NE(nullptr, route);
  EXPECT_EQ("cluster2", route->routeEntry()->clusterName());

  metadata.setMethodName("METHOD1");
  route = matcher.route(metadata, 0);
  EXPECT_NE(nullptr, route);
  EXPECT_EQ("cluster2", route->routeEntry()->clusterName());

  metadata.setMethodName("method1");
  route = matcher.route(metadata, 0);
  EXPECT_NE(nullptr, route);
  EXPECT_EQ("cluster1", route->routeEntry()->clusterName());

  metadata.setMethodName("method2");
  route = matcher.route(metadata, 0);
  EXPECT_EQ(nullptr, route);
}

TEST(RouteMatcherTest, RouteByAnyMethodNameWithNoInversion) {
  const std::string yaml = R"EOF(
name: config
routes:
  - match:
      method_name: "method1"
    route:
      cluster: "cluster1"
  - match:
      method_name: ""
    route:
      cluster: "cluster2"
)EOF";

  envoy::config::filter::network::thrift_proxy::v2alpha1::RouteConfiguration config =
      parseRouteConfigurationFromV2Yaml(yaml);

  RouteMatcher matcher(config);

  {
    MessageMetadata metadata;
    metadata.setMethodName("method1");
    RouteConstSharedPtr route = matcher.route(metadata, 0);
    EXPECT_NE(nullptr, route);
    EXPECT_EQ("cluster1", route->routeEntry()->clusterName());

    metadata.setMethodName("anything");
    RouteConstSharedPtr route2 = matcher.route(metadata, 0);
    EXPECT_NE(nullptr, route2);
    EXPECT_EQ("cluster2", route2->routeEntry()->clusterName());
  }

  {
    MessageMetadata metadata;
    RouteConstSharedPtr route2 = matcher.route(metadata, 0);
    EXPECT_NE(nullptr, route2);
    EXPECT_EQ("cluster2", route2->routeEntry()->clusterName());
  }
}

TEST(RouteMatcherTest, RouteByAnyMethodNameWithInversion) {
  const std::string yaml = R"EOF(
name: config
routes:
  - match:
      method_name: ""
      invert: true
    route:
      cluster: "cluster2"
)EOF";

  envoy::config::filter::network::thrift_proxy::v2alpha1::RouteConfiguration config =
      parseRouteConfigurationFromV2Yaml(yaml);

  EXPECT_THROW(new RouteMatcher(config), EnvoyException);
}

TEST(RouteMatcherTest, RouteByServiceNameWithNoInversion) {
  const std::string yaml = R"EOF(
name: config
routes:
  - match:
      method_name: "method1"
    route:
      cluster: "cluster1"
  - match:
      service_name: "service2"
    route:
      cluster: "cluster2"
)EOF";

  envoy::config::filter::network::thrift_proxy::v2alpha1::RouteConfiguration config =
      parseRouteConfigurationFromV2Yaml(yaml);

  RouteMatcher matcher(config);
  MessageMetadata metadata;
  EXPECT_EQ(nullptr, matcher.route(metadata, 0));
  metadata.setMethodName("unknown");
  EXPECT_EQ(nullptr, matcher.route(metadata, 0));
  metadata.setMethodName("METHOD1");
  EXPECT_EQ(nullptr, matcher.route(metadata, 0));

  metadata.setMethodName("service2:method1");
  RouteConstSharedPtr route = matcher.route(metadata, 0);
  EXPECT_NE(nullptr, route);
  EXPECT_EQ("cluster2", route->routeEntry()->clusterName());

  metadata.setMethodName("service2:method2");
  RouteConstSharedPtr route2 = matcher.route(metadata, 0);
  EXPECT_NE(nullptr, route2);
  EXPECT_EQ("cluster2", route2->routeEntry()->clusterName());
}

TEST(RouteMatcherTest, RouteByServiceNameWithInversion) {
  const std::string yaml = R"EOF(
name: config
routes:
  - match:
      method_name: "method1"
    route:
      cluster: "cluster1"
  - match:
      service_name: "service2"
      invert: true
    route:
      cluster: "cluster2"
)EOF";

  envoy::config::filter::network::thrift_proxy::v2alpha1::RouteConfiguration config =
      parseRouteConfigurationFromV2Yaml(yaml);

  RouteMatcher matcher(config);
  MessageMetadata metadata;
  RouteConstSharedPtr route = matcher.route(metadata, 0);
  EXPECT_NE(nullptr, route);
  EXPECT_EQ("cluster2", route->routeEntry()->clusterName());

  metadata.setMethodName("unknown");
  route = matcher.route(metadata, 0);
  EXPECT_NE(nullptr, route);
  EXPECT_EQ("cluster2", route->routeEntry()->clusterName());

  metadata.setMethodName("METHOD1");
  route = matcher.route(metadata, 0);
  EXPECT_NE(nullptr, route);
  EXPECT_EQ("cluster2", route->routeEntry()->clusterName());

  metadata.setMethodName("method1");
  route = matcher.route(metadata, 0);
  EXPECT_NE(nullptr, route);
  EXPECT_EQ("cluster1", route->routeEntry()->clusterName());

  metadata.setMethodName("service2:method1");
  route = matcher.route(metadata, 0);
  EXPECT_EQ(nullptr, route);
}

TEST(RouteMatcherTest, RouteByAnyServiceNameWithNoInversion) {
  const std::string yaml = R"EOF(
name: config
routes:
  - match:
      method_name: "method1"
    route:
      cluster: "cluster1"
  - match:
      service_name: ""
    route:
      cluster: "cluster2"
)EOF";

  envoy::config::filter::network::thrift_proxy::v2alpha1::RouteConfiguration config =
      parseRouteConfigurationFromV2Yaml(yaml);

  RouteMatcher matcher(config);

  {
    MessageMetadata metadata;
    metadata.setMethodName("method1");
    RouteConstSharedPtr route = matcher.route(metadata, 0);
    EXPECT_NE(nullptr, route);
    EXPECT_EQ("cluster1", route->routeEntry()->clusterName());

    metadata.setMethodName("anything");
    RouteConstSharedPtr route2 = matcher.route(metadata, 0);
    EXPECT_NE(nullptr, route2);
    EXPECT_EQ("cluster2", route2->routeEntry()->clusterName());
  }

  {
    MessageMetadata metadata;
    RouteConstSharedPtr route2 = matcher.route(metadata, 0);
    EXPECT_NE(nullptr, route2);
    EXPECT_EQ("cluster2", route2->routeEntry()->clusterName());
  }
}

TEST(RouteMatcherTest, RouteByAnyServiceNameWithInversion) {
  const std::string yaml = R"EOF(
name: config
routes:
  - match:
      service_name: ""
      invert: true
    route:
      cluster: "cluster2"
)EOF";

  envoy::config::filter::network::thrift_proxy::v2alpha1::RouteConfiguration config =
      parseRouteConfigurationFromV2Yaml(yaml);

  EXPECT_THROW(new RouteMatcher(config), EnvoyException);
}

TEST(RouteMatcherTest, RouteByExactHeaderMatcher) {
  const std::string yaml = R"EOF(
name: config
routes:
  - match:
      method_name: "method1"
      headers:
      - name: "x-header-1"
        exact_match: "x-value-1"
    route:
      cluster: "cluster1"
)EOF";

  envoy::config::filter::network::thrift_proxy::v2alpha1::RouteConfiguration config =
      parseRouteConfigurationFromV2Yaml(yaml);

  RouteMatcher matcher(config);
  MessageMetadata metadata;
  RouteConstSharedPtr route = matcher.route(metadata, 0);
  EXPECT_EQ(nullptr, route);

  metadata.setMethodName("method1");
  route = matcher.route(metadata, 0);
  EXPECT_EQ(nullptr, route);

  metadata.headers().addCopy(Http::LowerCaseString("x-header-1"), "x-value-1");
  route = matcher.route(metadata, 0);
  EXPECT_NE(nullptr, route);
  EXPECT_EQ("cluster1", route->routeEntry()->clusterName());
}

TEST(RouteMatcherTest, RouteByRegexHeaderMatcher) {
  const std::string yaml = R"EOF(
name: config
routes:
  - match:
      method_name: "method1"
      headers:
      - name: "x-version"
        regex_match: "0.[5-9]"
    route:
      cluster: "cluster1"
)EOF";

  envoy::config::filter::network::thrift_proxy::v2alpha1::RouteConfiguration config =
      parseRouteConfigurationFromV2Yaml(yaml);

  RouteMatcher matcher(config);
  MessageMetadata metadata;
  RouteConstSharedPtr route = matcher.route(metadata, 0);
  EXPECT_EQ(nullptr, route);

  metadata.setMethodName("method1");
  route = matcher.route(metadata, 0);
  EXPECT_EQ(nullptr, route);

  metadata.headers().addCopy(Http::LowerCaseString("x-version"), "0.1");
  route = matcher.route(metadata, 0);
  EXPECT_EQ(nullptr, route);
  metadata.headers().remove(Http::LowerCaseString("x-version"));

  metadata.headers().addCopy(Http::LowerCaseString("x-version"), "0.8");
  route = matcher.route(metadata, 0);
  EXPECT_NE(nullptr, route);
  EXPECT_EQ("cluster1", route->routeEntry()->clusterName());
}

TEST(RouteMatcherTest, RouteByRangeHeaderMatcher) {
  const std::string yaml = R"EOF(
name: config
routes:
  - match:
      method_name: "method1"
      headers:
      - name: "x-user-id"
        range_match:
          start: 100
          end: 200
    route:
      cluster: "cluster1"
)EOF";

  envoy::config::filter::network::thrift_proxy::v2alpha1::RouteConfiguration config =
      parseRouteConfigurationFromV2Yaml(yaml);

  RouteMatcher matcher(config);
  MessageMetadata metadata;
  RouteConstSharedPtr route = matcher.route(metadata, 0);
  EXPECT_EQ(nullptr, route);

  metadata.setMethodName("method1");
  route = matcher.route(metadata, 0);
  EXPECT_EQ(nullptr, route);

  metadata.headers().addCopy(Http::LowerCaseString("x-user-id"), "50");
  route = matcher.route(metadata, 0);
  EXPECT_EQ(nullptr, route);
  metadata.headers().remove(Http::LowerCaseString("x-user-id"));

  metadata.headers().addCopy(Http::LowerCaseString("x-user-id"), "199");
  route = matcher.route(metadata, 0);
  EXPECT_NE(nullptr, route);
  EXPECT_EQ("cluster1", route->routeEntry()->clusterName());
}

TEST(RouteMatcherTest, RouteByPresentHeaderMatcher) {
  const std::string yaml = R"EOF(
name: config
routes:
  - match:
      method_name: "method1"
      headers:
      - name: "x-user-id"
        present_match: true
    route:
      cluster: "cluster1"
)EOF";

  envoy::config::filter::network::thrift_proxy::v2alpha1::RouteConfiguration config =
      parseRouteConfigurationFromV2Yaml(yaml);

  RouteMatcher matcher(config);
  MessageMetadata metadata;
  RouteConstSharedPtr route = matcher.route(metadata, 0);
  EXPECT_EQ(nullptr, route);

  metadata.setMethodName("method1");
  route = matcher.route(metadata, 0);
  EXPECT_EQ(nullptr, route);

  metadata.headers().addCopy(Http::LowerCaseString("x-user-id"), "50");
  route = matcher.route(metadata, 0);
  EXPECT_NE(nullptr, route);
  EXPECT_EQ("cluster1", route->routeEntry()->clusterName());
  metadata.headers().remove(Http::LowerCaseString("x-user-id"));

  metadata.headers().addCopy(Http::LowerCaseString("x-user-id"), "");
  route = matcher.route(metadata, 0);
  EXPECT_NE(nullptr, route);
  EXPECT_EQ("cluster1", route->routeEntry()->clusterName());
}

TEST(RouteMatcherTest, RouteByPrefixHeaderMatcher) {
  const std::string yaml = R"EOF(
name: config
routes:
  - match:
      method_name: "method1"
      headers:
      - name: "x-header-1"
        prefix_match: "user_id:"
    route:
      cluster: "cluster1"
)EOF";

  envoy::config::filter::network::thrift_proxy::v2alpha1::RouteConfiguration config =
      parseRouteConfigurationFromV2Yaml(yaml);

  RouteMatcher matcher(config);
  MessageMetadata metadata;
  RouteConstSharedPtr route = matcher.route(metadata, 0);
  EXPECT_EQ(nullptr, route);

  metadata.setMethodName("method1");
  route = matcher.route(metadata, 0);
  EXPECT_EQ(nullptr, route);

  metadata.headers().addCopy(Http::LowerCaseString("x-header-1"), "500");
  route = matcher.route(metadata, 0);
  EXPECT_EQ(nullptr, route);
  metadata.headers().remove(Http::LowerCaseString("x-header-1"));

  metadata.headers().addCopy(Http::LowerCaseString("x-header-1"), "user_id:500");
  route = matcher.route(metadata, 0);
  EXPECT_NE(nullptr, route);
  EXPECT_EQ("cluster1", route->routeEntry()->clusterName());
}

TEST(RouteMatcherTest, RouteBySuffixHeaderMatcher) {
  const std::string yaml = R"EOF(
name: config
routes:
  - match:
      method_name: "method1"
      headers:
      - name: "x-header-1"
        suffix_match: "asdf"
    route:
      cluster: "cluster1"
)EOF";

  envoy::config::filter::network::thrift_proxy::v2alpha1::RouteConfiguration config =
      parseRouteConfigurationFromV2Yaml(yaml);

  RouteMatcher matcher(config);
  MessageMetadata metadata;
  RouteConstSharedPtr route = matcher.route(metadata, 0);
  EXPECT_EQ(nullptr, route);

  metadata.setMethodName("method1");
  route = matcher.route(metadata, 0);
  EXPECT_EQ(nullptr, route);

  metadata.headers().addCopy(Http::LowerCaseString("x-header-1"), "asdfvalue");
  route = matcher.route(metadata, 0);
  EXPECT_EQ(nullptr, route);
  metadata.headers().remove(Http::LowerCaseString("x-header-1"));

  metadata.headers().addCopy(Http::LowerCaseString("x-header-1"), "valueasdfvalue");
  route = matcher.route(metadata, 0);
  EXPECT_EQ(nullptr, route);
  metadata.headers().remove(Http::LowerCaseString("x-header-1"));

  metadata.headers().addCopy(Http::LowerCaseString("x-header-1"), "value:asdf");
  route = matcher.route(metadata, 0);
  EXPECT_NE(nullptr, route);
  EXPECT_EQ("cluster1", route->routeEntry()->clusterName());
}

TEST(RouteMatcherTest, WeightedClusters) {
  const std::string yaml = R"EOF(
name: config
routes:
  - match:
      method_name: "method1"
    route:
      weighted_clusters:
        clusters:
          - name: cluster1
            weight: 30
          - name: cluster2
            weight: 30
          - name: cluster3
            weight: 40
  - match:
      method_name: "method2"
    route:
      weighted_clusters:
        clusters:
          - name: cluster1
            weight: 2000
          - name: cluster2
            weight: 3000
          - name: cluster3
            weight: 5000
)EOF";

  envoy::config::filter::network::thrift_proxy::v2alpha1::RouteConfiguration config =
      parseRouteConfigurationFromV2Yaml(yaml);
  RouteMatcher matcher(config);
  MessageMetadata metadata;

  {
    metadata.setMethodName("method1");
    EXPECT_EQ("cluster1", matcher.route(metadata, 0)->routeEntry()->clusterName());
    EXPECT_EQ("cluster1", matcher.route(metadata, 29)->routeEntry()->clusterName());
    EXPECT_EQ("cluster2", matcher.route(metadata, 30)->routeEntry()->clusterName());
    EXPECT_EQ("cluster2", matcher.route(metadata, 59)->routeEntry()->clusterName());
    EXPECT_EQ("cluster3", matcher.route(metadata, 60)->routeEntry()->clusterName());
    EXPECT_EQ("cluster3", matcher.route(metadata, 99)->routeEntry()->clusterName());
    EXPECT_EQ("cluster1", matcher.route(metadata, 100)->routeEntry()->clusterName());
  }

  {
    metadata.setMethodName("method2");
    EXPECT_EQ("cluster1", matcher.route(metadata, 0)->routeEntry()->clusterName());
    EXPECT_EQ("cluster1", matcher.route(metadata, 1999)->routeEntry()->clusterName());
    EXPECT_EQ("cluster2", matcher.route(metadata, 2000)->routeEntry()->clusterName());
    EXPECT_EQ("cluster2", matcher.route(metadata, 4999)->routeEntry()->clusterName());
    EXPECT_EQ("cluster3", matcher.route(metadata, 5000)->routeEntry()->clusterName());
    EXPECT_EQ("cluster3", matcher.route(metadata, 9999)->routeEntry()->clusterName());
    EXPECT_EQ("cluster1", matcher.route(metadata, 10000)->routeEntry()->clusterName());
  }
}

TEST(RouteMatcherTest, WeightedClusterMissingWeight) {
  const std::string yaml = R"EOF(
name: config
routes:
  - match:
      method_name: "method2"
    route:
      weighted_clusters:
        clusters:
          - name: cluster1
            weight: 20000
          - name: cluster2
          - name: cluster3
            weight: 5000
)EOF";

  const envoy::config::filter::network::thrift_proxy::v2alpha1::RouteConfiguration config =
      parseRouteConfigurationFromV2Yaml(yaml);
  EXPECT_THROW(RouteMatcher m(config), EnvoyException);
}

} // namespace
} // namespace Router
} // namespace ThriftProxy
} // namespace NetworkFilters
} // namespace Extensions
} // namespace Envoy
