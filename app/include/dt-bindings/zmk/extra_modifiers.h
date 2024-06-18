/*
 * Copyright (c) 2020 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */
#pragma once

#define LHYP LS(LC(LA(LGUI)))
#define LHYPK(key) LS(LC(LA(LG(key))))

#define LMEH LS(LC(LALT))
#define LMEHK(key) LS(LC(LA(key)))

#define RHYP RS(RC(RA(RGUI)))
#define RHYPK(key) RS(RC(RA(RG(key))))

#define RMEH RS(RC(RALT))
#define RMEHK(key) RS(RC(RA(key)))

#define LSG LS(LGUI)
#define LSGK(key) LS(LG(key))

#define RSG RS(RGUI)
#define RSGK(key) RS(RG(key))

#define LSC LS(LCTRL)
#define LSCK(key) LS(LC(key))

#define RSC RS(RCTRL)
#define RSCK(key) RS(RC(key))

#define LSA LS(LALT)
#define LSAK(key) LS(LA(key))

#define RSA RS(RALT)
#define RSAK(key) RS(RA(key))

#define LCA LC(LALT)
#define LCAK(key) LC(LA(key))

#define RCA RC(RALT)
#define RCAK(key) RC(RA(key))

#define LCG LC(LGUI)
#define LCGK(key) LC(LG(key))

#define RCG RC(RGUI)
#define RCGK(key) RC(RG(key))

#define LAG LA(LGUI)
#define LAGK(key) LA(LG(key))

#define RAG RA(RGUI)
#define RAGK(key) RA(RG(key))
