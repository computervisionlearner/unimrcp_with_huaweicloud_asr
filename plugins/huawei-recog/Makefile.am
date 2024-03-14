AM_CPPFLAGS                = $(UNIMRCP_PLUGIN_INCLUDES)

plugin_LTLIBRARIES         = huaweirecog.la

huaweirecog_la_SOURCES       = src/huawei_recog_engine.cc
huaweirecog_la_LDFLAGS       = $(UNIMRCP_PLUGIN_OPTS) \
                               -L$(top_srcdir)/plugins/huawei-recog/huaweicloud_asr/third_party \
                               -L$(top_srcdir)/plugins/huawei-recog/huaweicloud_asr/third_party/jsoncpp-install/lib64 \
                               -L$(top_srcdir)/plugins/huawei-recog/huaweicloud_asr/third_party/gflags-install/lib    \
                               -L$(top_srcdir)/plugins/huawei-recog/huaweicloud_asr/third_party/glog-install/lib64    \
                               ${CMAKE_SOURCE_DIR}/third_party/openssl-install/lib                                    \
                               -lhuawei_rasr -ssl -lcrypto -ljsoncpp -lgflags_nothreads -stdc++=14
huaweirecog_ladir            = $(libdir)
huaweirecog_la_DATA          = $(top_srcdir)/plugins/huawei-recog/huaweicloud_asr/third_party/libhuawei_rasr.so       \
                               $(top_srcdir)/plugins/huawei-recog/huaweicloud_asr/third_party/openssl-install/lib/libcrypto.so  \
                               $(top_srcdir)/plugins/huawei-recog/huaweicloud_asr/third_party/openssl-install/lib/libssl.so     \
                               $(top_srcdir)/plugins/huawei-recog/huaweicloud_asr/third_party/jsoncpp-install/lib64/libjsoncpp.so \
                               $(top_srcdir)/plugins/huawei-recog/huaweicloud_asr/third_party/gflags-install/lib/libgflags_nothreads.so
                              

include $(top_srcdir)/build/rules/uniplugin.am

UNIMRCP_PLUGIN_INCLUDES      += -I$(top_srcdir)/plugins/huawei-recog/huaweicloud_asr/include  \
                                -I$(top_srcdir)/plugins/huawei-recog/huaweicloud_asr/third_party/websocketpp-src \
                                -I$(top_srcdir)/plugins/huawei-recog/huaweicloud_asr/third_party/jsoncpp-install/include     \
                                -I$(top_srcdir)/plugins/huawei-recog/huaweicloud_asr/third_party/boost-src/                 \
                                -I$(top_srcdir)/plugins/huawei-recog/huaweicloud_asr/third_party/openssl-install/include    \
                                -I$(top_srcdir)/plugins/huawei-recog/huaweicloud_asr/third_party/gflags-install/include
