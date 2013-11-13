# New ports collection makefile for:	confetti
# Date created:		2012-08-01
# Whom:			Gvozdikov Veniamin <g.veniamin@googlemail.com>
#
# $FreeBSD$
#

PORTNAME=	confetti
PORTVERSION=	0.0.${DATE}
CATEGORIES=	textproc
MASTER_SITES=	http://fbsd.zlonet.ru/distfiles/

MAINTAINER=	g.veniamin@googlemail.com
COMMENT=	Configuration file parser generator

BUILD_DEPENDS=	flex:${PORTSDIR}/textproc/flex \
		bison:${PORTSDIR}/devel/bison

USE_BZIP2=	yes
USE_GMAKE=	yes
USE_PERL5=	yes
DATE=		20120801
MAKE_ARGS+=	FLEX=${LOCALBASE}/bin/flex

OPTIONS_DEFINE=	EXAMPLES
EXAMPLES_DESC=	Install examples

OPTIONS_DEFAULT=EXAMPLES

.include <bsd.port.options.mk>

.if ${PORT_OPTIONS:MEXAMPLES}
PLIST_SUB+=	EXAMPLES=""
.else
PLIST_SUB+=	EXAMPLES="@comment "
.endif

do-install:
	${INSTALL} ${WRKSRC}/${PORTNAME} ${PREFIX}/bin/${PORTNAME}
post-install:
.if ${PORT_OPTIONS:MEXAMPLES}
	${MKDIR} ${EXAMPLESDIR}
	cd ${WRKSRC} && ${COPYTREE_SHARE} example ${EXAMPLESDIR}
.endif

.include <bsd.port.mk>
