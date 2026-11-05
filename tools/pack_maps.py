#!/usr/bin/env python3
"""Pack SDK-exported tiles and metadata from git-tracked assets/shores/*.tmx."""
from tmx_helpers import *
a=options(); maps=load_maps('shores/shore[0-9].tmx','SHORE_COUNT',(112,24))
lines=['#include "game.h"','const shore_t SHORES[SHORE_COUNT] = {']
routes=['#include "game.h"','#include "route.h"']
names=['Fish','Crab','Jellyfish','Gull','Buoy','Mother']
for i,m in enumerate(maps):
    write(a.out/f'shore{i}.ztm',m.tiles(a))
    objects=m.named_objects(['Player']+names)
    players=[o for o in objects if o['kind']=='Player']
    if len(players)!=1: raise ValueError(f'{m.path}: exactly one Player required')
    spawns=sorted([o for o in objects if o['kind']!='Player'],key=lambda o:(o['x'],o['y']))
    if len(spawns)>constant('MAX_SPAWNS') or not any(o['kind']=='Mother' for o in spawns): raise ValueError(f'{m.path}: invalid spawns')
    write(a.out/f'shore{i}.spn',bytes(v for o in spawns for v in [names.index(o['kind'])+1,o['x'],o['y'],int(o['props'].get('arg',0))]))
    p=players[0];fish=sum(o['kind']=='Fish' for o in spawns)
    lines.append('    { %d, %d, %d, %d, %s },' % (len(spawns),p['x'],p['y'],fish,c_string(m.props['name'])))
    route=sorted(m.named_objects(['Waypoint'],'Routes'),key=lambda o:o['props']['order'])
    routes.append(f'static const waypoint_t ROUTE{i}[] = {{')
    routes += ['    { %d, %d, %d },' % (o['x'],o['y'],o['props'].get('flags',0)) for o in route]
    routes += ['    { 0, 0, 0xff },','};']
lines.append('};')
routes += ['const waypoint_t *const ROUTES[] = { '+', '.join(f'ROUTE{i}' for i in range(len(maps)))+' };',f'const uint8_t ROUTE_COUNT = {len(maps)};']
write(a.out/'map_data.c','\n'.join(lines)+'\n');write(a.out/'map_routes.c','\n'.join(routes)+'\n')
