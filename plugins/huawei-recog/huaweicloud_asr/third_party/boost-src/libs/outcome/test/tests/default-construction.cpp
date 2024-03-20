/* Unit testing for outcomes
(C) 2013-2020 Niall Douglas <http://www.nedproductions.biz/> (4 commits)


Boost Software License - Version 1.0 - August 17th, 2003

Permission is hereby granted, free of charge, to any person or organization
obtaining a copy of the software and accompanying documentation covered by
this license (the "Software") to use, reproduce, display, distribute,
execute, and transmit the Software, and to prepare derivative works of the
Software, and to permit third-parties to whom the Software is furnished to
do so, all subject to the following:

The copyright notices in the Software and this entire statement, including
the above license grant, this restriction and the following disclaimer,
must be included in all copies of the Software, in whole or in part, and
all derivative works of the Software, unless such copies or derivative
works are solely in the form of machine-executable object code generated by
a source language processor.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE, TITLE AND NON-INFRINGEMENT. IN NO EVENT
SHALL THE COPYRIGHT HOLDERS OR ANYONE DISTRIBUTING THE SOFTWARE BE LIABLE
FOR ANY DAMAGES OR OTHER LIABILITY, WHETHER IN CONTRACT, TORT OR OTHERWISE,
ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
DEALINGS IN THE SOFTWARE.
*/

#include <boost/outcome/outcome.hpp>
#include <boost/test/unit_test.hpp>
#include <boost/test/unit_test_monitor.hpp>

BOOST_OUTCOME_AUTO_TEST_CASE(works_outcome_default_construction, "Tests that outcome default constructs when it ought to")
{
  using namespace BOOST_OUTCOME_V2_NAMESPACE;
  struct udt
  {
    int _v{78};
    udt() = default;
    constexpr explicit udt(int v) noexcept : _v(v) {}
    udt(udt &&o) = delete;
    udt(const udt &) = delete;
    udt &operator=(udt &&o) = delete;
    udt &operator=(const udt &) = delete;
    ~udt() = default;
    constexpr int operator*() const noexcept { return _v; }
  };
  // One path is via success_type<void>
  outcome<udt> a(success());
  BOOST_CHECK(*a.value() == 78);

  // Other path is via explicit conversion from void
  outcome<void> c = success();
  outcome<udt> d(c);
  BOOST_CHECK(*d.value() == 78);
}
