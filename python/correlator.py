import numpy as np
import pandas as pd
import os
from sklearn.preprocessing import PolynomialFeatures
from sklearn.linear_model import LinearRegression

FILE_PATH = os.path.join(os.path.dirname(__file__), "sensor_data.csv")

HEADER_PATH = os.path.join(os.path.dirname(__file__), "../src/virtual_tof_coeffs.h")

def generate_header(model1, model2, feature_names):
    def get_formula_str(model):
        coef = model.coef_
        intercept = model.intercept_
        terms = [f"{intercept:.6f}f"]
        for i, feat in enumerate(feature_names[1:]):
            c = coef[i+1]
            if abs(c) > 1e-11:
                fmt_feat = feat.replace(" ", " * ").replace("^2", "_sq")
                fmt_feat = fmt_feat.replace("s1s2", "s1 * s2")
                terms.append(f"({c:.10f}f * {fmt_feat})")
        return " + ".join(terms)

    t1_formula = get_formula_str(model1)
    t2_formula = get_formula_str(model2)

    header_content = f"""#ifndef VIRTUAL_TOF_COEFFS_H
#define VIRTUAL_TOF_COEFFS_H

// Auto-generated calibration coefficients
// Generated on: {pd.Timestamp.now()}
// Polynomial Degree: 2

inline float get_t1_est(float s1, float s2) {{
    float s1_sq = s1 * s1;
    float s2_sq = s2 * s2;
    return {t1_formula};
}}

inline float get_t2_est(float s1, float s2) {{
    float s1_sq = s1 * s1;
    float s2_sq = s2 * s2;
    return {t2_formula};
}}

#endif
"""
    with open(HEADER_PATH, 'w') as f:
        f.write(header_content)
    print(f"\n✅ Created header file: {HEADER_PATH}")

def derive_functions(show_plots=True):
    if not os.path.exists(FILE_PATH):
        print(f"File {FILE_PATH} not found!")
        return

    data = pd.read_csv(FILE_PATH)
    if data.empty:
        print("CSV is empty.")
        return

    data = data.dropna()
    X = data[['s1', 's2']].values
    y1 = data['t1'].values
    y2 = data['t2'].values

    poly = PolynomialFeatures(degree=2)
    X_poly = poly.fit_transform(X)

    model1 = LinearRegression().fit(X_poly, y1)
    model2 = LinearRegression().fit(X_poly, y2)

    feature_names = poly.get_feature_names_out(['s1', 's2'])
    
    print("Model R^2 Scores:")
    print(f"  T1: {model1.score(X_poly, y1):.4f}")
    print(f"  T2: {model2.score(X_poly, y2):.4f}")

    generate_header(model1, model2, feature_names)

    if not show_plots:
        return

    # --- Visualization ---
    import matplotlib.pyplot as plt
    
    y1_pred = model1.predict(X_poly)
    y2_pred = model2.predict(X_poly)

    fig, (ax1, ax2) = plt.subplots(1, 2, figsize=(12, 5))

    # Plot T1 Actual vs predicted
    ax1.scatter(y1, y1_pred, alpha=0.3, s=2, c='blue')
    ax1.plot([min(y1), max(y1)], [min(y1), max(y1)], 'r--', lw=2) # 45 degree line
    ax1.set_title(f"T1: Actual vs Predicted (R²={model1.score(X_poly, y1):.3f})")
    ax1.set_xlabel("True ToF Distance (mm)")
    ax1.set_ylabel("Predicted Distance (f(s1, s2))")
    ax1.grid(True)

    # Plot T2 Actual vs predicted
    ax2.scatter(y2, y2_pred, alpha=0.3, s=2, c='green')
    ax2.plot([min(y2), max(y2)], [min(y2), max(y2)], 'r--', lw=2) # 45 degree line
    ax2.set_title(f"T2: Actual vs Predicted (R²={model2.score(X_poly, y2):.3f})")
    ax2.set_xlabel("True ToF Distance (mm)")
    ax2.set_ylabel("Predicted Distance (f(s1, s2))")
    ax2.grid(True)

    plt.tight_layout()
    print("\nDisplaying Correlation Plots...")
    plt.show()

if __name__ == "__main__":
    derive_functions()
