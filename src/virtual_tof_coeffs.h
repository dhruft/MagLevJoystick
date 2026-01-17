#ifndef VIRTUAL_TOF_COEFFS_H
#define VIRTUAL_TOF_COEFFS_H

// Auto-generated calibration coefficients
// Generated on: 2026-01-16 15:35:10.781351
// Polynomial Degree: 2

inline float get_t1_est(float s1, float s2) {
    float s1_sq = s1 * s1;
    float s2_sq = s2 * s2;
    return -35.001870f + (0.0429769281f * s1) + (0.0435583123f * s2) + (-0.0000054849f * s1_sq) + (-0.0000127265f * s1 * s2) + (-0.0000040333f * s2_sq);
}

inline float get_t2_est(float s1, float s2) {
    float s1_sq = s1 * s1;
    float s2_sq = s2 * s2;
    return 139.873197f + (-0.0476692343f * s1) + (-0.0404498862f * s2) + (0.0000063287f * s1_sq) + (0.0000120940f * s1 * s2) + (0.0000029441f * s2_sq);
}

#endif
