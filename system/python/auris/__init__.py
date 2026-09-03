"""Auris prototype pipeline — Python host of the CV stack and SceneState transport.

All spatial math lives here; Unity is a dumb renderer fed by the frozen
SceneState UDP schema. Contracts live in ``auris.contracts`` and are shared by
Prototype and Production backends (transition-first, README §4).
"""

__version__ = "0.1.0"
