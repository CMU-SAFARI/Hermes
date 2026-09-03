#!/usr/bin/env bash
# Set up the R environment for house-style plotting and verify it end-to-end.
# Idempotent: skips anything already present. Safe to re-run.
#
# Usage:   bash setup_r_env.sh            # install-if-missing + verify
#          bash setup_r_env.sh --verify   # verify only, install nothing
#
# On a locked-down / offline machine the package install may fail (no CRAN
# mirror). See the MANUAL FALLBACK printed at the end if so.
set -uo pipefail

PKGS="ggplot2 hrbrthemes dplyr tidyr yaml ragg scales"
VERIFY_ONLY=0
[ "${1:-}" = "--verify" ] && VERIFY_ONLY=1

# 1. R present?
if ! command -v Rscript >/dev/null 2>&1; then
  echo "FAIL: Rscript not found. Install R first (e.g. apt-get install r-base, or conda install r-base)." >&2
  exit 1
fi
echo "OK  : $(Rscript --version 2>&1 | head -1)"

# 2. Packages present? Install missing ones (unless --verify).
MISSING=$(Rscript -e "cat(setdiff(strsplit('$PKGS',' ')[[1]], rownames(installed.packages())))" 2>/dev/null)
if [ -n "$MISSING" ]; then
  if [ "$VERIFY_ONLY" = 1 ]; then
    echo "MISS: packages not installed: $MISSING"
  else
    echo "INFO: installing missing packages: $MISSING"
    Rscript -e "install.packages(strsplit('$MISSING',' ')[[1]], repos='https://cloud.r-project.org')" \
      || echo "WARN: package install failed (see MANUAL FALLBACK below)"
  fi
else
  echo "OK  : all packages present ($PKGS)"
fi

# 3. Roboto Condensed font (hrbrthemes dependency). Register via hrbrthemes if
#    not already available to the graphics devices.
Rscript -e '
  ok <- tryCatch("Roboto Condensed" %in% systemfonts::system_fonts()$family, error=function(e) FALSE)
  if (!ok && requireNamespace("hrbrthemes", quietly=TRUE)) {
    try(hrbrthemes::import_roboto_condensed(), silent=TRUE)
    ok <- tryCatch("Roboto Condensed" %in% systemfonts::system_fonts()$family, error=function(e) FALSE)
  }
  cat(if (ok) "OK  : Roboto Condensed available\n" else
      "WARN: Roboto Condensed not found (charts render but the font falls back)\n")
' 2>/dev/null

# 4. Smoke-render a one-bar chart through BOTH devices.
TMP=$(mktemp -d)
Rscript -e "
  suppressPackageStartupMessages({library(ggplot2); library(hrbrthemes); library(ragg)})
  p <- ggplot(data.frame(x='a', y=1), aes(x,y)) + geom_col() + theme_ipsum_rc()
  ggsave('$TMP/smoke.png', p, width=4, height=3, dpi=100, device=ragg::agg_png)
  ggsave('$TMP/smoke.pdf', p, width=4, height=3, device=grDevices::cairo_pdf)
" 2>/dev/null
if [ -s "$TMP/smoke.png" ] && [ -s "$TMP/smoke.pdf" ]; then
  echo "OK  : smoke-render passed (ragg PNG + cairo PDF)"
  rm -rf "$TMP"
  echo "READY: house-style plotting environment verified."
  exit 0
else
  echo "FAIL: smoke-render did not produce both outputs." >&2
  rm -rf "$TMP"
  cat >&2 <<'EOF'

--- MANUAL FALLBACK ---
If package install failed (offline / no CRAN mirror):
  * install from your distro:  apt-get install r-cran-ggplot2 r-cran-dplyr r-cran-tidyr r-cran-yaml
  * ragg/scales/hrbrthemes may need a mirror; set one with
      Rscript -e "install.packages('ragg', repos='<internal-mirror>')"
  * ragg PNG unavailable? Fall back to device=grDevices::png (lower quality) but keep cairo_pdf.
  * font missing? charts still render with the default font; ignore the WARN.
EOF
  exit 1
fi
