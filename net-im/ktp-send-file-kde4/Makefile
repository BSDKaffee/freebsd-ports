# $FreeBSD$

PORTNAME=	ktp-send-file
PORTVERSION=	${KTP_VERSION}
PORTREVISION=	3
CATEGORIES=	net-im kde
MASTER_SITES=	KDE/${KTP_BRANCH}/kde-telepathy/${PORTVERSION}/src
PKGNAMESUFFIX=	-kde4

MAINTAINER=	kde@FreeBSD.org
COMMENT=	KDE instant messaging file transfer application

LIB_DEPENDS=	libtelepathy-qt4.so:net-im/telepathy-qt4 \
		libktpcommoninternalsprivate.so:net-im/ktp-common-internals-kde4
RUN_DEPENDS=	${KDE_PREFIX}/lib/kde4/libexec/ktp-filetransfer-handler:net-im/ktp-filetransfer-handler-kde4 \
		${KDE_PREFIX}/bin/ktp-contactlist:net-im/ktp-contact-list-kde4

USES=		cmake gettext kde:4 qt:4 tar:bzip2
USE_KDE=	kdelibs automoc4
USE_QT=		corelib moc_build qmake_build rcc_build uic_build

.include <bsd.port.mk>
