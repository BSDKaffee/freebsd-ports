# Created by: Mark Felder <feld@FreeBSD.org>
# $FreeBSD$

PORTNAME=	snort
DISTVERSION=	3.0.0-a4.243
PORTREVISION=	1
CATEGORIES=	security
PKGNAMESUFFIX=	3

MAINTAINER=	feld@FreeBSD.org
COMMENT=	Lightweight network intrusion detection system

LICENSE=	GPLv2
LICENSE_FILE=	${WRKSRC}/LICENSE

BUILD_DEPENDS=	daq>=2.2.2:net/daq \
		bash>0:shells/bash
LIB_DEPENDS=	libpcre.so:devel/pcre \
		libdnet.so:net/libdnet \
		libpcap.so:net/libpcap \
		libluajit-5.1.so:lang/luajit \
		libhwloc.so:devel/hwloc

RUN_DEPENDS=	daq>=2.2.2:net/daq

USES=		cmake compiler:c++11-lang cpe libtool pathfix pkgconfig ssl iconv
LDFLAGS+=	-L${LOCALBASE}/lib ${ICONV_LIB}
USE_GITHUB=	yes
GH_ACCOUNT=	snortadmin
GH_PROJECT=	snort3
GH_TAGNAME=	BUILD_243

USE_LDCONFIG=   yes

OPTIONS_DEFINE=		ADDRESSSANITIZER DEBUG DOCS LARGEPCAP PIE THREADSANITIZER STATICDAQ
OPTIONS_DEFINE_amd64= 	HYPERSCAN TSC
OPTIONS_DEFAULT=	DOCS STATICDAQ

ADDRESSSANITIZER_DESC=	Enable address sanitizer
DEBUG_DESC=		Enable debug
DOCS_DESC=		Enable documentation
# FLATBUFFERS_DESC=	Enable flatbuffers support      // Doesn't get detected
HYPERSCAN_DESC=		Enable high-performance regular expression lib
LARGEPCAP_DESC=		Enable support for pcaps larger than 2 GB
PIE_DESC=		Enable PIE (Broken)
STATICDAQ_DESC=		Link static DAQ modules
THREADSANITIZER_DESC=	Enable thread sanitizer
TSC_DESC=		Use timestamp counter register clock (x86 only)

ADDRESSSANITIZER_CMAKE_BOOL= ENABLE_ADDRESS_SANITIZER
DEBUG_CMAKE_BOOL=	ENABLE_COREFILES ENABLE_GDB
DOCS_CMAKE_BOOL=	MAKE_HTML_DOC MAKE_PDF_DOC MAKE_TEXT_DOC
LARGEPCAP_CMAKE_BOOL=	ENABLE_LARGE_PCAP
PIE_CMAKE_BOOL=		ENABLE_PIE
STATICDAQ_CMAKE_BOOL=	ENABLE_STATIC_DAQ
THREADSANITIZER_CMAKE_BOOL= ENABLE_THREAD_SANITIZER
TSC_CMAKE_BOOL=		ENABLE_TSC_CLOCK

# FLATBUFFERS_LIB_DEPENDS=	libflatbuffers.so:devel/flatbuffers
HYPERSCAN_LIB_DEPENDS=	libhs.so:devel/hyperscan

CONFLICTS=	snort-2*

.include <bsd.port.mk>
