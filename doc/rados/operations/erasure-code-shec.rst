========================
SHEC erasure code plugin
========================

The *shec* plugin encapsulates the `multiple SHEC
<https://wiki.ceph.com/Planning/Blueprints/Hammer/Shingled_Erasure_Code_(SHEC)>`_
library. It allows ceph to recover data more efficiently than Reed Solomon.

Create an SHEC profile
======================

To create a new *shec* erasure code profile::

        ceph osd erasure-code-profile set {name} \
             plugin=shec \
             [k={data-chunks}] \
             [m={coding-chunks}] \
             [c={durability-estimator}] \
             [ruleset-root={root}] \
             [ruleset-failure-domain={bucket-type}] \
             [directory={directory}] \
             [--force]

Where:

``k={data-chunks}``

:Description: Each object is split in **data-chunks** parts,
              each stored on a different OSD.

:Type: Integer
:Required: No.
:Default: 4

``m={coding-chunks}``

:Description: Compute **coding-chunks** for each object and store them on
              different OSDs. The number of **coding-chunks** is not necessarily
              the number of OSDs that can be down without losing data.

:Type: Integer
:Required: No.
:Default: 3

``c={durability-estimator}``

:Description: The number of parity chunks that cover each data chunk in a
              calculation range, which is used as an estimator of durability.
              For instance, if c is 2, 2 OSDs can be down without losing data.

:Type: Integer
:Required: No.
:Default: 2

``ruleset-root={root}``

:Description: The name of the crush bucket used for the first step of
              the ruleset. For intance **step take default**.

:Type: String
:Required: No.
:Default: default

``ruleset-failure-domain={bucket-type}``

:Description: Ensure that no two chunks are in a bucket with the same
              failure domain. For instance, if the failure domain is
              **host** no two chunks will be stored on the same
              host. It is used to create a ruleset step such as **step
              chooseleaf host**.

:Type: String
:Required: No.
:Default: host

``directory={directory}``

:Description: Set the **directory** name from which the erasure code
              plugin is loaded.

:Type: String
:Required: No.
:Default: /usr/lib/ceph/erasure-code

``--force``

:Description: Override an existing profile by the same name.

:Type: String
:Required: No.

Brief description of SHEC's layouts
===================================

Space Efficiency
----------------

Space efficiency is a ratio of data chunks to all ones in a object and
represented as k/(k+m).
In order to improve space efficiency, you should increase k or decrease m.

        space efficiency of SHEC(4,3,2) = 4/(4+3) = 0.57

Durability
----------

The third parameter (=c) of SHEC is a durability estimator, which approximates
the number of OSDs that can be down without losing data.

        durability of SHEC(4,3,2) = 2

Recovery Efficiency
-------------------

Recovery efficiency cannot be calculated so easily, but increasing m without
increasing c achieves improvement of recovery efficiency.
(However, we must pay attention to the sacrifice of space efficiency in this case.)

        layout change of SHEC(4,2,2) -> SHEC(4,3,2) gains more recovery efficiency

Erasure code profile examples
=============================

        $ ceph osd erasure-code-profile set SHECprofile \
             plugin=shec \
             k=8 m=4 c=3 \
             ruleset-failure-domain=host
        $ ceph osd pool create shecpool 256 256 erasure SHECprofile
