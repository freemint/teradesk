#!/bin/sh

set -e

mkdir -p "$BUILDROOT/DOC/DE"
mkdir -p "$BUILDROOT/DOC/RU"
mkdir -p "$BUILDROOT/RSC/DE"
mkdir -p "$BUILDROOT/RSC/EN"
mkdir -p "$BUILDROOT/RSC/IT"
mkdir -p "$BUILDROOT/RSC/FR"
mkdir -p "$BUILDROOT/RSC/RU"

#
# Common files
#
mcopy -b "$SRCDIR/desktop.prg" "$BUILDROOT/DESKTOP.PRG"
mcopy -b "$SRCDIR/desktos.prg" "$BUILDROOT/DESKTOS.PRG"
test -f desk_cf.prg && cp -a desk_cf.prg "$BUILDROOT/DESK_CF.PRG"
cp -a "desktop.rsc" "$BUILDROOT/DESKTOP.RSC"
cp -a "cicons.rsc" "$BUILDROOT/CICONS.RSC"
cp -a "icons.rsc" "$BUILDROOT/ICONS.RSC"
cp -a "readme.txt" "$BUILDROOT/README.TXT"
cp -a "COPYING" "$BUILDROOT/DOC"
cp -a "doc/hist_v34.txt" "$BUILDROOT/DOC/HIST_V34.TXT"
cp -a "rsc/resource.txt" "$BUILDROOT/RSC/RESOURCE.TXT"

#
# English localizations
#
lang=en
cp -a "desktop.rsc" "$BUILDROOT/RSC/EN/DESKTOP.RSC"
cp -a "desktop.rso" "$BUILDROOT/RSC/EN/DESKTOP.RSO"
cp -a "desktop.hrd" "$BUILDROOT/RSC/EN/DESKTOP.HRD"
cp -a "doc/${lang}/teradesk.hyp" "$BUILDROOT/DOC/TERADESK.HYP"
cp -a "doc/${lang}/teradesk.ref" "$BUILDROOT/DOC/TERADESK.REF"

#
# German localizations
#
lang=de
cp -a "rsc/$lang/desktop.rsc" "$BUILDROOT/RSC/DE/DESKTOP.RSC"
cp -a "rsc/$lang/desktop.rso" "$BUILDROOT/RSC/DE/DESKTOP.RSO"
cp -a "rsc/$lang/desktop.hrd" "$BUILDROOT/RSC/DE/DESKTOP.HRD"
cp -a "rsc/$lang/readme.txt" "$BUILDROOT/RSC/DE/README.TXT"
cp -a "doc/${lang}/teradesk.hyp" "$BUILDROOT/DOC/DE/TERADESK.HYP"
cp -a "doc/${lang}/teradesk.ref" "$BUILDROOT/DOC/DE/TERADESK.REF"

#
# Russian localizations
#
lang=ru
cp -a "rsc/$lang/desktop.rsc" "$BUILDROOT/RSC/RU/DESKTOP.RSC"
cp -a "rsc/$lang/desktop.rso" "$BUILDROOT/RSC/RU/DESKTOP.RSO"
cp -a "rsc/$lang/desktop.hrd" "$BUILDROOT/RSC/RU/DESKTOP.HRD"
cp -a "rsc/$lang/readme.txt" "$BUILDROOT/RSC/RU/README.TXT"
cp -a "rsc/$lang/read_eng.txt" "$BUILDROOT/RSC/RU/READ_ENG.TXT"
cp -a "doc/${lang}/teradesk.hyp" "$BUILDROOT/DOC/RU/TERADESK.HYP"
cp -a "doc/${lang}/teradesk.ref" "$BUILDROOT/DOC/RU/TERADESK.REF"

#
# French localizations
#
lang=fr
cp -a "rsc/$lang/desktop.rsc" "$BUILDROOT/RSC/FR/DESKTOP.RSC"
cp -a "rsc/$lang/desktop.rso" "$BUILDROOT/RSC/FR/DESKTOP.RSO"
cp -a "rsc/$lang/desktop.hrd" "$BUILDROOT/RSC/FR/DESKTOP.HRD"
cp -a "rsc/$lang/lisezmoi.txt" "$BUILDROOT/RSC/FR/LISEZMOI.TXT"

#
# Italian localizations
#
lang=it
cp -a "rsc/$lang/desktop.rsc" "$BUILDROOT/RSC/IT/DESKTOP.RSC"
cp -a "rsc/$lang/desktop.rso" "$BUILDROOT/RSC/IT/DESKTOP.RSO"
cp -a "rsc/$lang/desktop.hrd" "$BUILDROOT/RSC/IT/DESKTOP.HRD"
cp -a "rsc/$lang/copy_ita.txt" "$BUILDROOT/RSC/IT/COPY_ITA.TXT"
cp -a "rsc/$lang/leggimi.txt" "$BUILDROOT/RSC/IT/LEGGIMI.TXT"
cp -a "rsc/$lang/readme.txt" "$BUILDROOT/RSC/IT/README.TXT"


set +e
