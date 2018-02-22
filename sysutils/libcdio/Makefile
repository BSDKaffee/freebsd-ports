# Created by: arved
# $FreeBSD$

PORTNAME=	libcdio
PORTVERSION=	2.0.0
CATEGORIES=	sysutils
MASTER_SITES=	GNU

MAINTAINER=	jhale@FreeBSD.org
COMMENT=	Compact Disc Input and Control Library

LICENSE=	GPLv3+
LICENSE_FILE=	${WRKSRC}/COPYING

LIB_DEPENDS=	libcddb.so:audio/libcddb

USES=		cpe gmake iconv localbase libtool \
		ncurses pathfix perl5 pkgconfig
USE_PERL5=	build
USE_LDCONFIG=	yes

GNU_CONFIGURE=	yes
CONFIGURE_ARGS=	${ICONV_CONFIGURE_ARG}
LIBS+=		${ICONV_LIB}

INFO=		libcdio
CPE_VENDOR=	gnu
INSTALL_TARGET=	install-strip
TEST_TARGET=	check

.include <bsd.port.mk>
