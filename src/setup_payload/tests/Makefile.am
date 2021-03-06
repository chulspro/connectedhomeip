#
#    Copyright (c) 2020 Project CHIP Authors
#
#    Licensed under the Apache License, Version 2.0 (the "License");
#    you may not use this file except in compliance with the License.
#    You may obtain a copy of the License at
#
#        http://www.apache.org/licenses/LICENSE-2.0
#
#    Unless required by applicable law or agreed to in writing, software
#    distributed under the License is distributed on an "AS IS" BASIS,
#    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#    See the License for the specific language governing permissions and
#    limitations under the License.
#

#
#    Description:
#      This is the GNU automake file for the CHIP QR code unit
#      and functional tests.
#

include $(abs_top_nlbuild_autotools_dir)/automake/pre.am

#
# Local headers to build against and distribute but not to install
# since they are not part of the package.
#
noinst_HEADERS                                 = \
    $(NULL)

#
# Other files we do want to distribute with the package.
#
EXTRA_DIST                                     = \
    $(NULL)

if CHIP_BUILD_TESTS
lib_LIBRARIES                                  = \
    libSetupPayloadTests.a                       \
    $(NULL)

libSetupPayloadTests_a_SOURCES                 = \
    TestManualCode.cpp                           \
    TestQRCode.cpp                               \
    TestQRCodeTLV.cpp                            \
    $(NULL)

libSetupPayloadTests_adir                      = $(includedir)/setup_payload

dist_libSetupPayloadTests_a_HEADERS            = \
    TestHelpers.h                                \
    TestManualCode.h                             \
    TestQRCode.h                                 \
    TestQRCodeTLV.h                              \
    $(NULL)

# C/C++ preprocessor option flags that will apply to all compiled
# objects in this makefile.

AM_CPPFLAGS                                           = \
    -I$(top_srcdir)/src                                 \
    -I$(top_srcdir)/src/lib                             \
    -I$(top_srcdir)/src/lib/core                        \
    -I$(top_srcdir)/src/system                          \
    -I$(top_srcdir)/src/setup_payload                   \
    $(LWIP_CPPFLAGS)                                    \
    $(NLASSERT_CPPFLAGS)                                \
    $(NLIO_CPPFLAGS)                                    \
    $(NLUNIT_TEST_CPPFLAGS)                             \
    $(SOCKETS_CPPFLAGS)                                 \
    $(PTHREAD_CFLAGS)                                   \
    $(NULL)

CHIP_LDADD                                            = \
    $(top_builddir)/src/lib/libCHIP.a                   \
    $(top_builddir)/src/lib/support/libSupportLayer.a   \
    $(NULL)

COMMON_LDADD                                           = \
    libSetupPayloadTests.a                               \
    $(COMMON_LDFLAGS)                                    \
    $(CHIP_LDADD)                                        \
    $(NLUNIT_TEST_LDFLAGS) $(NLUNIT_TEST_LIBS)           \
    $(NLFAULTINJECTION_LDFLAGS) $(NLFAULTINJECTION_LIBS) \
    $(LWIP_LDFLAGS) $(LWIP_LIBS)                         \
    $(SOCKETS_LDFLAGS) $(SOCKETS_LIBS)                   \
    $(PTHREAD_CFLAGS) $(PTHREAD_LIBS)                    \
    $(NULL)

# Test applications that should be run when the 'check' target is run.

check_PROGRAMS                                 = \
    TestManualCode                               \
    TestQRCode                                   \
    TestQRCodeTLV                                \
    $(NULL)

# Test applications and scripts that should be built and run when the
# 'check' target is run.

TESTS                                          = \
    $(check_PROGRAMS)                            \
    $(NULL)

# The additional environment variables and their values that will be
# made available to all programs and scripts in TESTS.

TESTS_ENVIRONMENT                              = \
    $(NULL)

# Source, compiler, and linker options for test programs.

TestQRCode_LDADD                               = $(COMMON_LDADD)
TestQRCode_SOURCES                             = TestQRCodeDriver.cpp

TestManualCode_LDADD                           = $(COMMON_LDADD)
TestManualCode_SOURCES                         = TestManualCodeDriver.cpp

TestQRCodeTLV_LDADD                            = $(COMMON_LDADD)
TestQRCodeTLV_SOURCES                          = TestQRCodeTLVDriver.cpp

#
# Foreign make dependencies
#

NLFOREIGN_FILE_DEPENDENCIES                           = \
   $(CHIP_LDADD)                                        \
   $(NULL)

NLFOREIGN_SUBDIR_DEPENDENCIES                         = \
   $(NLFAULTINJECTION_FOREIGN_SUBDIR_DEPENDENCY)        \
   $(NLUNIT_TEST_FOREIGN_SUBDIR_DEPENDENCY)             \
   $(NULL)

if CHIP_BUILD_COVERAGE
CLEANFILES                                     = $(wildcard *.gcda *.gcno)

if CHIP_BUILD_COVERAGE_REPORTS
# The bundle should positively be qualified with the absolute build
# path. Otherwise, VPATH will get auto-prefixed to it if there is
# already such a directory in the non-colocated source tree.

CHIP_COVERAGE_BUNDLE                           = ${abs_builddir}/${PACKAGE}${NL_COVERAGE_BUNDLE_SUFFIX}
CHIP_COVERAGE_INFO                             = ${CHIP_COVERAGE_BUNDLE}/${PACKAGE}${NL_COVERAGE_INFO_SUFFIX}

$(CHIP_COVERAGE_BUNDLE):
	$(call create-directory)

$(CHIP_COVERAGE_INFO): check-local | $(CHIP_COVERAGE_BUNDLE)
	$(call generate-coverage-report,${top_builddir},*/usr/include/* */third_party/*)

coverage-local: $(CHIP_COVERAGE_INFO)

clean-local: clean-local-coverage

.PHONY: clean-local-coverage
clean-local-coverage:
	-$(AM_V_at)rm -rf $(CHIP_COVERAGE_BUNDLE)
endif # CHIP_BUILD_COVERAGE_REPORTS
endif # CHIP_BUILD_COVERAGE
endif # CHIP_BUILD_TESTS

include $(abs_top_nlbuild_autotools_dir)/automake/post.am
