content = [
    ".vu",
    ".global PantheonShader",
    "PantheonShader:",
    "    nop                             lq VF01, 2(VI00)",
    "    nop                             lq VF02, 3(VI00)",
    "    nop                             lq VF03, 4(VI00)",
    "    nop                             lq VF04, 5(VI00)",
    "    nop                             lq VF05, 1(VI00)",
    "    nop                             lq VF06, 0(VI00)",
    "    nop                             sq VF06, 64(VI00)",
    "    nop                             xitop VI01",
    "    nop                             iaddiu VI02, VI00, 6",
    "    nop                             iaddiu VI03, VI00, 65",
    "VertexLoop:",
    "    nop                             ibeq VI01, VI00, EndLoop",
    "    nop                             iaddi VI01, VI01, -1",
    "    nop                             lq VF08, 0(VI02)",
    "    nop                             lq VF09, 1(VI02)",
    "    nop                             iaddi VI02, VI02, 2",
    "    mulax ACC, VF01, VF08x          nop",
    "    madday ACC, VF02, VF08y         nop",
    "    maddaz ACC, VF03, VF08z         nop",
    "    maddw VF10, VF04, VF08w         nop",
    "    nop                             div q, VF00w, VF10w",
    "    nop                             waitq",
    "    mulq.xyzw VF10, VF10, q         nop",
    "    mulaw.xyzw ACC, VF05, VF00w     nop",
    "    madd.xyzw VF10, VF10, VF05      nop",   # ← this exact form is what dvp-as accepts
    "    ftoi4.xyzw VF10, VF10           nop",
    "    nop                             sq VF09, 0(VI03)",
    "    nop                             sq VF10, 1(VI03)",
    "    nop                             iaddi VI03, VI03, 2",
    "    nop                             b VertexLoop",
    "    nop                             nop",
    "EndLoop:",
    "    nop                             iaddiu VI04, VI00, 64",
    "    nop                             xgkick VI04",
    "    nop[E]                          nop",
    "    nop                             nop",
    ".global PantheonShaderEnd",
    "PantheonShaderEnd:"
]

with open('shader.vsm', 'w', encoding='ascii') as f:
    f.write('\n'.join(content) + '\n')

print("✅ shader.vsm generated with Quake-II-proven post-perspective madd pattern")
