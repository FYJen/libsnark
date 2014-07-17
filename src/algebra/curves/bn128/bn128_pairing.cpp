/** @file
 ********************************************************************************
 Implements functions for computing Ate pairings over the bn128 curves, split into a
 offline and online stages.

 This code is based on the ate-pairing pairing code (https://github.com/herumi/ate-pairing),
 split into offline and online stages.
 ********************************************************************************
 * @author     This file is part of libsnark, developed by SCIPR Lab
 *             and contributors (see AUTHORS).
 * Uses portions of ate-pairing <https://github.com/herumi/ate-pairing> by
 *             MITSUNARI Shigeo and TERUYA Tadanori
 * @copyright  modified new BSD License (http://opensource.org/licenses/BSD-3-Clause)
 *             (see LICENSE file)
 *******************************************************************************/

/*
The "bn128" curve choice included in libsnark is based on the ate-pairing library by
MITSUNARI Shigeo and TERUYA Tadanori [https://github.com/herumi/ate-pairing].

The ate-pairing library is licensed under "modified new BSD License"
[http://opensource.org/licenses/BSD-3-Clause].

While the ate-pairing library does not include explicit license text, the license
template at the link above is:

Copyright (c) <YEAR>, <OWNER>
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are
met:

1. Redistributions of source code must retain the above copyright
notice, this list of conditions and the following disclaimer.

2. Redistributions in binary form must reproduce the above copyright
notice, this list of conditions and the following disclaimer in the
documentation and/or other materials provided with the distribution.

3. Neither the name of the copyright holder nor the names of its
contributors may be used to endorse or promote products derived from
this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
"AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/
#include "algebra/curves/bn128/bn128_pairing.hpp"
#include "common/profiling.hpp"
#include "algebra/curves/bn128/bn128_init.hpp"
#include "algebra/curves/bn128/bn128_g1.hpp"
#include "algebra/curves/bn128/bn128_g2.hpp"
#include "algebra/curves/bn128/bn128_gt.hpp"

namespace libsnark {

typedef bn::Fp Fp;
typedef bn::Fp2 Fp2;
typedef bn::Fp2Dbl Fp2Dbl;
typedef bn::ParamT<Fp2> Param;
typedef bn::Fp6T<bn::Fp2> Fp6;
typedef bn::Fp12T<bn::Fp6> Fp12;

bool bn128_ate_G1_precomp::operator==(const bn128_ate_G1_precomp &other) const
{
    return (this->P[0] == other.P[0] &&
            this->P[1] == other.P[1] &&
            this->P[2] == other.P[2]);
}

std::ostream& operator<<(std::ostream &out, const bn128_ate_G1_precomp &prec_P)
{
    for (size_t i = 0; i < 3; ++i)
    {
#ifndef BINARY_OUTPUT
        out << prec_P.P[i] << "\n";
#else
        out.write((char*) &prec_P.P[i], sizeof(prec_P.P[i]));
#endif
    }
    return out;
}

std::istream& operator>>(std::istream &in, bn128_ate_G1_precomp &prec_P)
{
    for (size_t i = 0; i < 3; ++i)
    {
#ifndef BINARY_OUTPUT
        in >> prec_P.P[i];
        consume_newline(in);
#else
        in.read((char*) &prec_P.P[i], sizeof(prec_P.P[i]));
#endif
    }
    return in;
}

bool bn128_ate_G2_precomp::operator==(const bn128_ate_G2_precomp &other) const
{
    return (this->Q[0] == other.Q[0] &&
            this->Q[1] == other.Q[1] &&
            this->Q[2] == other.Q[2] &&
            this->coeffs == other.coeffs);
}

std::ostream& operator<<(std::ostream &out, const bn128_ate_G2_precomp &prec_Q)
{
    for (size_t i = 0; i < 3; ++i)
    {
#ifndef BINARY_OUTPUT
        out << prec_Q.Q[i].a_ << "\n";
        out << prec_Q.Q[i].b_ << "\n";
#else
        out.write((char*) &prec_Q.Q[i].a_, sizeof(prec_Q.Q[i].a_));
        out.write((char*) &prec_Q.Q[i].b_, sizeof(prec_Q.Q[i].b_));
#endif
    }

    out << prec_Q.coeffs.size() << "\n";

    for (size_t i = 0; i < prec_Q.coeffs.size(); ++i)
    {
#ifndef BINARY_OUTPUT
        out << prec_Q.coeffs[i].a_.a_ << "\n";
        out << prec_Q.coeffs[i].a_.b_ << "\n";
        out << prec_Q.coeffs[i].b_.a_ << "\n";
        out << prec_Q.coeffs[i].b_.b_ << "\n";
        out << prec_Q.coeffs[i].c_.a_ << "\n";
        out << prec_Q.coeffs[i].c_.b_ << "\n";
#else
        out.write((char*) &prec_Q.coeffs[i].a_.a_, sizeof(prec_Q.coeffs[i].a_.a_));
        out.write((char*) &prec_Q.coeffs[i].a_.b_, sizeof(prec_Q.coeffs[i].a_.b_));
        out.write((char*) &prec_Q.coeffs[i].b_.a_, sizeof(prec_Q.coeffs[i].b_.a_));
        out.write((char*) &prec_Q.coeffs[i].b_.b_, sizeof(prec_Q.coeffs[i].b_.b_));
        out.write((char*) &prec_Q.coeffs[i].c_.a_, sizeof(prec_Q.coeffs[i].c_.a_));
        out.write((char*) &prec_Q.coeffs[i].c_.b_, sizeof(prec_Q.coeffs[i].c_.b_));
#endif
    }

    return out;
}

std::istream& operator>>(std::istream &in, bn128_ate_G2_precomp &prec_Q)
{
    for (size_t i = 0; i < 3; ++i)
    {
#ifndef BINARY_OUTPUT
        in >> prec_Q.Q[i].a_;
        consume_newline(in);
        in >> prec_Q.Q[i].b_;
        consume_newline(in);
#else
        in.read((char*) &prec_Q.Q[i].a_, sizeof(prec_Q.Q[i].a_));
        in.read((char*) &prec_Q.Q[i].b_, sizeof(prec_Q.Q[i].b_));
#endif
    }

    size_t count;
    in >> count;
    consume_newline(in);
    prec_Q.coeffs.resize(count);
    for (size_t i = 0; i < count; ++i)
    {
#ifndef BINARY_OUTPUT
        in >> prec_Q.coeffs[i].a_.a_;
        consume_newline(in);
        in >> prec_Q.coeffs[i].a_.b_;
        consume_newline(in);
        in >> prec_Q.coeffs[i].b_.a_;
        consume_newline(in);
        in >> prec_Q.coeffs[i].b_.b_;
        consume_newline(in);
        in >> prec_Q.coeffs[i].c_.a_;
        consume_newline(in);
        in >> prec_Q.coeffs[i].c_.b_;
        consume_newline(in);
#else
        in.read((char*) &prec_Q.coeffs[i].a_.a_, sizeof(prec_Q.coeffs[i].a_.a_));
        in.read((char*) &prec_Q.coeffs[i].a_.b_, sizeof(prec_Q.coeffs[i].a_.b_));
        in.read((char*) &prec_Q.coeffs[i].b_.a_, sizeof(prec_Q.coeffs[i].b_.a_));
        in.read((char*) &prec_Q.coeffs[i].b_.b_, sizeof(prec_Q.coeffs[i].b_.b_));
        in.read((char*) &prec_Q.coeffs[i].c_.a_, sizeof(prec_Q.coeffs[i].c_.a_));
        in.read((char*) &prec_Q.coeffs[i].c_.b_, sizeof(prec_Q.coeffs[i].c_.b_));
#endif
    }
    return in;
}

void doubling_step_for_flipped_miller_loop(Fp2 *current, bn128_ate_ell_coeffs &l)
{
	Fp6::pointDblLineEvalWithoutP(l, current);
}

void mixed_addition_step_for_flipped_miller_loop(const Fp2* Q,
                                                 Fp2 *R,
                                                 bn128_ate_ell_coeffs &l)
{
	Fp6::pointAddLineEvalWithoutP(l, R, Q);
}

bn128_ate_G1_precomp bn128_ate_precompute_G1(const bn128_G1& P)
{
    enter_block("Call to bn128_ate_precompute_G1");

    bn128_ate_G1_precomp result;
    bn::ecop::NormalizeJac(result.P, P.coord);

    leave_block("Call to bn128_ate_precompute_G1");
    return result;
}

bn128_ate_G2_precomp bn128_ate_precompute_G2(const bn128_G2& Q)
{
    enter_block("Call to bn128_ate_precompute_G2");

    bn128_ate_G2_precomp result;
	bn::experimental::precomputeG2(result.coeffs, result.Q, Q.coord);

    leave_block("Call to bn128_ate_precompute_G2");
    return result;
}

bn128_Fq12 bn128_ate_miller_loop(const bn128_ate_G1_precomp &prec_P,
                                 const bn128_ate_G2_precomp &prec_Q)
{
    bn128_Fq12 f;
	bn::experimental::millerLoop(f.elem, prec_Q.coeffs, prec_P.P);
	return f;
}

bn128_Fq12 bn128_double_ate_miller_loop(const bn128_ate_G1_precomp &prec_P1,
                                        const bn128_ate_G2_precomp &prec_Q1,
                                        const bn128_ate_G1_precomp &prec_P2,
                                        const bn128_ate_G2_precomp &prec_Q2)
{
    bn128_Fq12 f;
	bn::experimental::millerLoop2(f.elem, prec_Q1.coeffs, prec_P1.P, prec_Q2.coeffs, prec_P2.P);
	return f;
}

bn128_GT bn128_final_exponentiation(const bn128_Fq12 &elt)
{
    enter_block("Call to bn128_final_exponentiation");
    bn128_GT eltcopy = elt;
    eltcopy.elem.final_exp();
    leave_block("Call to bn128_final_exponentiation");
    return eltcopy;
}
} // libsnark
