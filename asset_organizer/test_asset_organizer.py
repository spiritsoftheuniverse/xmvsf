"""Standard-library-only regression tests for local review saving and sequences."""
from __future__ import annotations
import hashlib
from http.server import ThreadingHTTPServer
import json
from pathlib import Path
import struct
import tempfile
from threading import Thread
import unittest
from urllib.request import Request, urlopen
from urllib.error import HTTPError

from SERVE_ASSET_ORGANIZER import make_handler, save_review, validate_review
from BUILD_WANTED_SEQUENCES import build


class AssetOrganizerTests(unittest.TestCase):
    def fixture(self, folder):
        root=Path(folder)
        prog=bytearray(0x400)
        for offset in (0x40,0x56,0x6c):
            struct.pack_into('<5H',prog,offset,2,1,2,0x35,0xF801)
        trace=root/'trace'/'run_1'
        trace.mkdir(parents=True)
        (trace/'mvc_program_data_host_words.bin').write_bytes(prog)
        digest=hashlib.sha256(prog).hexdigest()
        entries=[{'id':f'rom_{a:06X}','offset':f'{a:06X}'} for a in (0x40,0x56,0x6c)]
        catalog={'romSha256':digest,'catalog':entries}
        review={'format':'MVC1_ASSET_ORGANIZER_V2','sourceRomSha256':digest,
                'sourceCatalogSize':len(entries),
                'items':[{'id':e['id'],'romOffsetHex':e['offset'],
                         'wanted':i!=1,'reviewStatus':'UNWANTED' if i==1 else 'WANTED',
                         'tags':['effect'] if i else ['character','effect']}
                        for i,e in enumerate(entries)]}
        return trace.parent,catalog,review

    def test_strict_review_validation_and_atomic_save(self):
        with tempfile.TemporaryDirectory() as t:
            root=Path(t)
            trace, catalog, review=self.fixture(root)
            p=root/'aodata.json'
            save_review(p,review,catalog)
            self.assertEqual(json.loads(p.read_text()),review)
            wrong=json.loads(json.dumps(review));wrong['items'][0]['tags']=['not_a_tag']
            with self.assertRaisesRegex(ValueError,'invalid asset tags'):save_review(p,wrong,catalog)
            self.assertEqual(json.loads(p.read_text()),review)
            wrong=json.loads(json.dumps(review));wrong['items'][0]['id']=wrong['items'][1]['id'];wrong['items'][0]['romOffsetHex']=wrong['items'][1]['romOffsetHex']
            with self.assertRaisesRegex(ValueError,'duplicate'):validate_review(wrong,catalog)
            self.assertEqual(list(root.glob('*.tmp')),[])

    def test_wanted_structure_clustering(self):
        with tempfile.TemporaryDirectory() as t:
            root=Path(t)
            trace,catalog,review=self.fixture(root)
            reviewfile=root/'aodata.json';save_review(reviewfile,review,catalog)
            result=build(reviewfile,trace,root/'wanted_sequences.json',max_gap=1)
            self.assertEqual(result['wanted'],2)
            self.assertEqual(result['matchedWanted'],2)
            self.assertEqual(result['candidateGroupCount'],1)
            self.assertEqual(result['groups'][0]['wantedAssetIds'],['rom_000040','rom_00006C'])
            self.assertEqual(result['records'][0]['gapToNextWanted'],1)
            self.assertTrue((root/'wanted_sequences.tsv').exists())

    def test_http_only_loopback_file_and_json_api(self):
        with tempfile.TemporaryDirectory() as t:
            root=Path(t)
            trace,catalog,review=self.fixture(root)
            content=json.dumps(catalog,separators=(',',':'))
            (root/'index.html').write_text('<script type="application/json" id="original-catalog">'+content+'</script>')
            server=ThreadingHTTPServer(('127.0.0.1',0),make_handler(root,catalog))
            th=Thread(target=server.serve_forever,daemon=True);th.start()
            url=f'http://127.0.0.1:{server.server_port}/api/review'
            body=json.dumps(review).encode()
            def req(origin):
                return Request(url,method='POST',data=body,headers={
                    'Origin':origin,'Content-Type':'application/json'})
            try:
                self.assertEqual(urlopen(req(f'http://127.0.0.1:{server.server_port}')).status,200)
                self.assertEqual(json.loads(urlopen(url).read()),review)
                with self.assertRaises(HTTPError) as ex:urlopen(req('https://another-site.example'))
                self.assertEqual(ex.exception.code,403)
                with self.assertRaises(HTTPError) as ex:urlopen(url.replace('/api/review','/../../private'))
                self.assertEqual(ex.exception.code,404)
            finally:
                server.shutdown();server.server_close();th.join()

if __name__=='__main__':unittest.main()
