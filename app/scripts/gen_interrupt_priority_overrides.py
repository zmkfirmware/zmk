#!/usr/bin/env python
"""
Script for generating interrupt adjustments for zephyr dts files.

Requirements:
    - pip install devicetree
"""

import argparse
from devicetree import dtlib


def get_interrupt_nodes(dt):
    interrupt_nodes = []
    for node in dt.node_iter():
        if 'interrupts' in node.props.keys():
            interrupt_nodes.append(node)

    return interrupt_nodes

def get_node_label(node):
    label = node.labels[0]
    return label

def gen_interrupt_adjustment_str(node, priority_incr):
    label = get_node_label(node)
    interrupt_val1, interrupt_priority = node.props['interrupts'].to_nums()

    if label == 'gpiote':
        new_priority = 0
    else:
        new_priority = interrupt_priority + priority_incr

    adj_str = f'''
&{label} {{
    interrupts = < {interrupt_val1} {new_priority} >;
}};
    '''
    return adj_str


def gen_interrupt_adjustment(dts_path, priority_incr):
    dt = dtlib.DT(dts_path)
    interrupt_nodes = get_interrupt_nodes(dt)

    adjustments = ''
    for node in interrupt_nodes:
        adj_str = gen_interrupt_adjustment_str(node, priority_incr)
        if get_node_label(node) == 'gpiote':
            adjustments = adj_str + adjustments
        else:
            adjustments += adj_str

    return adjustments


def main():
    parser = argparse.ArgumentParser(description='Generate interrupt adjustments for zephyr.dts files.')
    parser.add_argument('dts_path', metavar='DTS_PATH', type=str, help='Path to the zephyr.dts file')
    parser.add_argument('-p', '--priority-incr', type=int, default=2, help='By how much to increase the priority (default: 2)')
    args = parser.parse_args()

    adj = gen_interrupt_adjustment(args.dts_path, args.priority_incr)
    print(adj)


if __name__ == '__main__':
    main()
