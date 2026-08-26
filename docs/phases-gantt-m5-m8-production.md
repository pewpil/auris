# Auris — Gantt: Months 5–8 · Production Stage & Evaluation

Full-time development, Dec 2026 – Apr 30, 2027. Companion chart: [`phases-gantt-m1-m4-prototype.md`](phases-gantt-m1-m4-prototype.md). Phase 4 is deliberately stretched across the Philippine December holidays; late April is defense-preparation buffer.

```mermaid
gantt
    title Auris — Months 5-8 · Production & Evaluation (Dec 2026 - Apr 2027)
    dateFormat YYYY-MM-DD
    axisFormat %b %y

    section Transition
    Gear arrival & assembly (ordered Nov 16)                  :asm, 2026-12-07, 12d

    section Phase 4 — Production CV pipeline (holiday-stretched)
    Multi-camera room-scale world-frame calibration           :p4a, after asm, 16d
    YOLO re-training on production captures + TriangulatedLocalizer :p4b, after p4a, 14d
    IMU-fused 6DoF head pose + handoff stability              :p4c, after p4b, 8d
    CV accuracy validation vs tape/ARUCO ground truth         :p4d, after p4c, 5d
    Milestone — production perception verified                :m2, after p4d, 0d

    section Phase 5 — Production audio & integration (3 wks)
    Engine port behind contracts + full-room deployment       :p5a, after m2, 12d
    Latency verification per interconnect path                :p5b, after p5a, 7d
    Milestone — system verified at room scale                 :m3, after p5b, 0d

    section Phase 6 — User study & thesis writing (8 wks)
    Pilot sessions + participant scheduling (ethics cleared since Sep) :p6a, after m3, 7d
    Study data collection                                     :p6b, after p6a, 14d
    Statistical analysis (ANOVA / t-tests)                    :p6c, after p6b, 10d
    Thesis writing + revisions                                :p6d, after p6c, 20d
    Buffer — defense preparation                              :buf, after p6d, 12d
    Milestone — thesis complete                               :m4, 2027-04-30, 0d
```
