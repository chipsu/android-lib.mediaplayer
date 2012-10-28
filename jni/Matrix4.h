// Copyright (c) 2012 - Use of this source code is governed by a BSD-style license that can be found in the LICENSE file.
#pragma once

struct Matrix4 {
	float m[4][4];
	
	Matrix4() {
		setIdentity();
	}
	
	Matrix4(float m11, float m12, float m13, float m14,
			float m21, float m22, float m23, float m24,
			float m31, float m32, float m33, float m34,
			float m41, float m42, float m43, float m44) {
		m[0][0] = m11; m[0][1] = m21; m[0][2] = m31; m[0][3] = m41;
		m[1][0] = m12; m[1][1] = m22; m[1][2] = m32; m[1][3] = m42;
		m[2][0] = m13; m[2][1] = m23; m[2][2] = m33; m[2][3] = m43;
		m[3][0] = m14; m[3][1] = m24; m[3][2] = m34; m[3][3] = m44;
	}
	
	explicit Matrix4(bool init) {
		if(init) {
			setIdentity();
		}
	}
	
	const float *ptr() const {
		return &m[0][0];
	}
	
	void setIdentity() {
		m[0][0] = 1.0f; m[0][1] = 0.0f; m[0][2] = 0.0f; m[0][3] = 0.0f;
		m[1][0] = 0.0f; m[1][1] = 1.0f; m[1][2] = 0.0f; m[1][3] = 0.0f;
		m[2][0] = 0.0f; m[2][1] = 0.0f; m[2][2] = 1.0f; m[2][3] = 0.0f;
		m[3][0] = 0.0f; m[3][1] = 0.0f; m[3][2] = 0.0f; m[3][3] = 1.0f;
	}

	float& operator()(int row, int column) {
		//assert(row >= 0 && row < 4 && column >= 0 && column < 4);
		return m[column][row];
	}
	
	friend Matrix4 operator * (const Matrix4& m1, const Matrix4& m2);
};

inline Matrix4 operator * (const Matrix4& m1, const Matrix4& m2) {
	Matrix4 m(false);
	m.m[0][0] = m1.m[0][0] * m2.m[0][0] +
				m1.m[1][0] * m2.m[0][1] +
				m1.m[2][0] * m2.m[0][2] +
				m1.m[3][0] * m2.m[0][3];
	m.m[0][1] = m1.m[0][1] * m2.m[0][0] +
				m1.m[1][1] * m2.m[0][1] +
				m1.m[2][1] * m2.m[0][2] +
				m1.m[3][1] * m2.m[0][3];
	m.m[0][2] = m1.m[0][2] * m2.m[0][0] +
				m1.m[1][2] * m2.m[0][1] +
				m1.m[2][2] * m2.m[0][2] +
				m1.m[3][2] * m2.m[0][3];
	m.m[0][3] = m1.m[0][3] * m2.m[0][0] +
				m1.m[1][3] * m2.m[0][1] +
				m1.m[2][3] * m2.m[0][2] +
				m1.m[3][3] * m2.m[0][3];
	m.m[1][0] = m1.m[0][0] * m2.m[1][0] +
				m1.m[1][0] * m2.m[1][1] +
				m1.m[2][0] * m2.m[1][2] +
				m1.m[3][0] * m2.m[1][3];
	m.m[1][1] = m1.m[0][1] * m2.m[1][0] +
				m1.m[1][1] * m2.m[1][1] +
				m1.m[2][1] * m2.m[1][2] +
				m1.m[3][1] * m2.m[1][3];
	m.m[1][2] = m1.m[0][2] * m2.m[1][0] +
				m1.m[1][2] * m2.m[1][1] +
				m1.m[2][2] * m2.m[1][2] +
				m1.m[3][2] * m2.m[1][3];
	m.m[1][3] = m1.m[0][3] * m2.m[1][0] +
				m1.m[1][3] * m2.m[1][1] +
				m1.m[2][3] * m2.m[1][2] +
				m1.m[3][3] * m2.m[1][3];
	m.m[2][0] = m1.m[0][0] * m2.m[2][0] +
				m1.m[1][0] * m2.m[2][1] +
				m1.m[2][0] * m2.m[2][2] +
				m1.m[3][0] * m2.m[2][3];
	m.m[2][1] = m1.m[0][1] * m2.m[2][0] +
				m1.m[1][1] * m2.m[2][1] +
				m1.m[2][1] * m2.m[2][2] +
				m1.m[3][1] * m2.m[2][3];
	m.m[2][2] = m1.m[0][2] * m2.m[2][0] +
				m1.m[1][2] * m2.m[2][1] +
				m1.m[2][2] * m2.m[2][2] +
				m1.m[3][2] * m2.m[2][3];
	m.m[2][3] = m1.m[0][3] * m2.m[2][0] +
				m1.m[1][3] * m2.m[2][1] +
				m1.m[2][3] * m2.m[2][2] +
				m1.m[3][3] * m2.m[2][3];
	m.m[3][0] = m1.m[0][0] * m2.m[3][0] +
				m1.m[1][0] * m2.m[3][1] +
				m1.m[2][0] * m2.m[3][2] +
				m1.m[3][0] * m2.m[3][3];
	m.m[3][1] = m1.m[0][1] * m2.m[3][0] +
				m1.m[1][1] * m2.m[3][1] +
				m1.m[2][1] * m2.m[3][2] +
				m1.m[3][1] * m2.m[3][3];
	m.m[3][2] = m1.m[0][2] * m2.m[3][0] +
				m1.m[1][2] * m2.m[3][1] +
				m1.m[2][2] * m2.m[3][2] +
				m1.m[3][2] * m2.m[3][3];
	m.m[3][3] = m1.m[0][3] * m2.m[3][0] +
				m1.m[1][3] * m2.m[3][1] +
				m1.m[2][3] * m2.m[3][2] +
				m1.m[3][3] * m2.m[3][3];
	return m;
}

