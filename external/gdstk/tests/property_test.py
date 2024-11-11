#!/usr/bin/env python

# Copyright 2020 Lucas Heitzmann Gabrielli.
# This file is part of gdstk, distributed under the terms of the
# Boost Software License - Version 1.0.  See the accompanying
# LICENSE file or <http://www.boost.org/LICENSE_1_0.txt>

import gdstk


def test_gds_properties():
    for obj in [
        gdstk.Polygon([-1 + 0j, -2j, 3 + 0j, 4j]),
        gdstk.FlexPath((0j, 1j), 0.1),
        gdstk.RobustPath(0j, 0.1),
        gdstk.Label("Label", 0j),
        gdstk.Reference("EMPTY"),
    ]:
        assert obj.get_gds_property(12) is None
        assert obj.delete_gds_property(12) is obj
        obj.set_gds_property(13, "Property text")
        assert obj.get_gds_property(12) is None
        assert obj.get_gds_property(13) == "Property text"
        obj.delete_gds_property(13)
        assert obj.get_gds_property(13) is None
        obj.set_gds_property(13, "Second text")
        obj.set_gds_property(13, "Third text")
        obj.set_gds_property(14, "Fourth text")
        assert obj.get_gds_property(13) == "Third text"
        assert obj.properties == [
            ["S_GDS_PROPERTY", 14, b"Fourth text\x00"],
            ["S_GDS_PROPERTY", 13, b"Third text\x00"],
        ]


def test_properties():
    for obj in [
        gdstk.Polygon([-1 + 0j, -2j, 3 + 0j, 4j]),
        gdstk.FlexPath((0j, 1j), 0.1),
        gdstk.RobustPath(0j, 0.1),
        gdstk.Label("Label", 0j),
        gdstk.Reference("EMPTY"),
        gdstk.Cell("CELL"),
        gdstk.Library("Name"),
    ]:
        assert len(obj.properties) == 0
        assert obj.get_property("None") is None
        obj.set_property("FIRST", 1)
        obj.set_property("SECOND", 2.0)
        obj.set_property("THIRD", -3)
        obj.set_property("FOURTH", [1, 2.0, -3, "FO", b"UR\x00TH\x00"])
        obj.set_property("FIRST", -1)
        assert obj.get_property("FIRST") == [-1]
        obj.delete_property("THIRD")
        assert obj.properties == [
            ["FIRST", -1],
            ["FOURTH", 1, 2.0, -3, b"FO", b"UR\x00TH\x00"],
            ["SECOND", 2.0],
            ["FIRST", 1],
        ]
        obj.properties = (
            ("ONE", -1),
            ("TWO", -2.3e-4, "two"),
            ("Three", b"\xFF\xEE", 0),
        )
        assert obj.properties == [
            ["ONE", -1],
            ["TWO", -2.3e-4, b"two"],
            ["Three", b"\xFF\xEE", 0],
        ]
