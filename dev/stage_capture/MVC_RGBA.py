"""Dependency-free CPS2 tile RGBA compositor and PNG encoder.

Pure stdlib. Original pixels only; no Pillow or NumPy, no network/ROM access.
"""
from __future__ import annotations
import struct
import zlib

class RasterError(ValueError): pass

class RGBAImage:
    """Minimal RGBA canvas; CPS2 pen 0 is transparent, all other pens opaque.

    The sprites are binary-alpha native art. Blitting nontransparent pixels
    therefore has the same result as RGBA source-over compositing while
    requiring no external imaging packages.
    """
    def __init__(self, width:int, height:int, rgba:bytes|bytearray|None=None):
        if not (0 < width <= 1600 and 0 < height <= 500):
            raise RasterError('invalid sprite dimensions')
        self.width=width; self.height=height
        self.size=(width,height)
        self.pixels=bytearray(rgba) if rgba is not None else bytearray(width*height*4)
        if len(self.pixels)!=width*height*4:
            raise RasterError('invalid RGBA byte count')

    def alpha_composite(self, image:'RGBAImage', dest:tuple[int,int]):
        left,top=dest
        if left<0 or top<0 or left+image.width>self.width or top+image.height>self.height:
            raise RasterError('sprite part outside composition bounds')
        out=self.pixels; src=image.pixels
        for row in range(image.height):
            si=row*image.width*4
            di=((top+row)*self.width+left)*4
            for x in range(image.width):
                a=si+x*4
                if src[a+3]:
                    b=di+x*4
                    out[b:b+4]=src[a:a+4]

    def getbbox(self):
        p=self.pixels; w=self.width
        xmin=w; xmax=-1; ymin=self.height; ymax=-1
        for i in range(3,len(p),4):
            if p[i]:
                pos=i//4; x=pos%w; y=pos//w
                if x<xmin:xmin=x
                if x>xmax:xmax=x
                if y<ymin:ymin=y
                if y>ymax:ymax=y
        return (xmin,ymin,xmax+1,ymax+1) if xmax>=0 else None

    def crop(self, box):
        x0,y0,x1,y1=box
        w=x1-x0; h=y1-y0
        p=bytearray(w*h*4)
        for y in range(h):
            start=((y0+y)*self.width+x0)*4
            p[y*w*4:(y+1)*w*4]=self.pixels[start:start+w*4]
        return RGBAImage(w,h,p)

    def png_bytes(self):
        """PNG RGBA8 with scanline filter 0 and standard zlib compression."""
        def chunk(kind,data):
            return struct.pack('>I',len(data))+kind+data+struct.pack('>I',zlib.crc32(kind+data)&0xffffffff)
        stride=self.width*4
        rows=b''.join(b'\x00'+self.pixels[y*stride:(y+1)*stride] for y in range(self.height))
        return (b'\x89PNG\r\n\x1a\n'+
                chunk(b'IHDR',struct.pack('>IIBBBBB',self.width,self.height,8,6,0,0,0))+
                chunk(b'IDAT',zlib.compress(rows,6))+chunk(b'IEND',b''))


def decode_original_tile(packed:bytes,pens:tuple[bytes,...],flipx:bool,flipy:bool)->RGBAImage:
    """Original CPS2 4bpp 16x16 packed-tile nibble order, no image packages."""
    if len(packed)!=128 or len(pens)!=16:
        raise RasterError('bad CPS2 tile byte length or palette')
    pixels=bytearray(16*16*4)
    for y in range(16):
        sy=15-y if flipy else y
        for x in range(16):
            sx=15-x if flipx else x
            offset=sy*8+(sx//8)*4+3-((sx%8)//2)
            value=packed[offset]
            pen=value>>4 if sx%2==0 else value&15
            at=(y*16+x)*4
            pixels[at:at+4]=pens[pen]
    return RGBAImage(16,16,pixels)

