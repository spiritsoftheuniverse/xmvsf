"""Standard-library-only tests: recover unobserved source artwork and native foreground."""
from __future__ import annotations
import csv
import gzip
import hashlib
import json
from pathlib import Path
import struct
import tempfile
import unittest

from BUILD_SOURCE_FAMILIES import build as recover_families, RecoveryError
from BUILD_FOREGROUND_MAPS import (map_at,decode_layer_tile,paint_layer,
                                   scan_original_foreground_history)


class SourceFamilyTests(unittest.TestCase):
    def fixture(self,root:Path):
        trace=root/'trace'/'run_0001';trace.mkdir(parents=True)
        prog=bytearray(0x400)
        for k,addr in enumerate((0x40,0x56,0x6c)):
            struct.pack_into('<5H',prog,addr,2,1,2,0x35,0xF801)
            for i in range(3):struct.pack_into('<HH',prog,addr+10+4*i,0x10+k*3+i,1)
        (trace/'mvc_program_data_host_words.bin').write_bytes(prog)
        digest=hashlib.sha256(prog).hexdigest()
        rev={'format':'MVC1_ASSET_ORGANIZER_V2','sourceRomSha256':digest,
             'sourceCatalogSize':2,'items':[
                 {'id':'rom_000040','romOffsetHex':'000040','wanted':True,'tags':['level_sprite'],
                  'sourceShot':'shot_0001','sourceFrame':100},
                 {'id':'rom_000056','romOffsetHex':'000056','wanted':False,'tags':[],
                  'sourceShot':'shot_0001','sourceFrame':120}]}
        review=root/'aodata.json';review.write_text(json.dumps(rev))
        observations=root/'sprites.tsv.gz'
        with gzip.open(observations,'wt',encoding='utf-8',newline='') as fp:
            w=csv.DictWriter(fp,fieldnames=('tile','attributes'),delimiter='\t');w.writeheader()
            for tile in (0x10010,0x10011,0x10012):w.writerow({'tile':tile,'attributes':1})
        return trace.parent,review,observations,prog

    def test_full_original_region_expansion_does_not_reaccept_rejected(self):
        with tempfile.TemporaryDirectory() as folder:
            root=Path(folder);trace,review,obs,original=self.fixture(root)
            result=recover_families(review,trace,obs,None,root/'output',previews=False)
            self.assertEqual(result['selected'],1)
            self.assertEqual(result['sourceRegions'],1)
            self.assertEqual(result['regionsOriginalRecords'],3)
            self.assertEqual(result['sourceDiscovered'],1)
            self.assertEqual(result['explicitUnwantedInRegions'],1)
            self.assertEqual(result['uniqueOriginal16x16TileIds'],9)
            ids={r['id']:r for r in result['records']}
            self.assertEqual(ids['rom_000040']['status'],'WANTED')
            self.assertEqual(ids['rom_000056']['status'],'UNWANTED')
            self.assertEqual(ids['rom_000056']['semanticType'],'UI')
            self.assertEqual(ids['rom_00006C']['status'],'SOURCE_DISCOVERED')
            self.assertIsNone(ids['rom_00006C']['explicitDecision'])
            self.assertEqual(ids['rom_00006C']['tileBankHigh'],1)
            self.assertEqual((trace/'run_0001'/'mvc_program_data_host_words.bin').read_bytes(),original)
            self.assertTrue((root/'output'/'family_recovery.tsv').is_file())

    def test_checksum_guard_prevents_mixing_rom_versions(self):
        with tempfile.TemporaryDirectory() as folder:
            root=Path(folder);trace,review,obs,program=self.fixture(root)
            blob=json.loads(review.read_text());blob['sourceRomSha256']='0'*64
            review.write_text(json.dumps(blob))
            with self.assertRaisesRegex(RecoveryError,'checksum'):
                recover_families(review,trace,obs,None,root/'output',previews=False)
            self.assertFalse((root/'output').exists())


class ForegroundTests(unittest.TestCase):
    def test_native_map_byte_addressing_unique_all_layers(self):
        for layer in (1,2,3):
            offsets={map_at(layer,x,y) for x in range(64) for y in range(64)}
            self.assertEqual(len(offsets),4096)
            self.assertEqual(min(offsets),0)
            self.assertEqual(max(offsets),16380)

    def test_original_8px_foreground_tile_decode_and_viewport(self):
        gfx=bytes([0x11]*4096)
        palette=bytearray(6144)
        idx=((0x20|0)*16+1)^15
        struct.pack_into('<H',palette,idx*2,0x0F00)
        tile=decode_layer_tile(gfx,len(gfx)-1,0,0,0,bytes(palette),1)
        self.assertEqual(len(tile),8*8*4)
        self.assertEqual(tile[3],255)
        self.assertGreater(tile[0],0)
        self.assertEqual(tile[1],0)
        bitmap,positions=paint_layer(bytes(0x4000),1,0,0,gfx,len(gfx)-1,0,bytes(palette))
        self.assertEqual(len(bitmap),384*224*4)
        self.assertTrue(positions)
        self.assertEqual(positions[0]['mapX'],0)
        self.assertEqual(positions[0]['mapY'],0)
        self.assertEqual(positions[0]['originalMapByteOffset'],0)

    def test_historical_map_variants_fetch_uncaptured_tile_cells(self):
        with tempfile.TemporaryDirectory() as folder:
            root=Path(folder);session=root/'capture'/'session_0001';session.mkdir(parents=True)
            (session/'frames.tsv').write_text('frame\tscroll1x\n100\t0\n150\t0\n',encoding='utf-8')
            page1=bytes(0x4000);page2=bytearray(page1)
            struct.pack_into('<HH',page2,0,5,0)
            with (session/'maps.bin').open('wb') as fp:
                for f,body in ((100,page1),(150,bytes(page2))):
                    fp.write(b'MAP1'+struct.pack('<IIIIIQ',f,0,0,1,0,0))
                    fp.write(body)
            ao={'items':[{'sourceShot':'shot_0001','sourceFrame':100},
                         {'sourceShot':'shot_0001','sourceFrame':150}]}
            result,tiles=scan_original_foreground_history(root/'capture',['session_0001'],
                                            {'shot_0001':[100,150]},ao)
            self.assertEqual(result['totalNativeScroll1MapSnapshotsScanned'],2)
            self.assertEqual(result['totalMapCellChangeEvents'],1)
            self.assertEqual(result['totalChangedMapCells'],1)
            self.assertEqual(tiles,{0,5})
            one=result['shots'][0]['changedCells'][0]
            self.assertEqual(one['mapByteOffset'],0)
            self.assertEqual([v['tileId'] for v in one['variants']],[0,5])


if __name__=='__main__':unittest.main()