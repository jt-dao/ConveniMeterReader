import requests
import png
from bs4 import BeautifulSoup
from PIL import Image, ImageOps
import base64

import pytesseract
import numpy as np
import cv2

import gspread
import datetime

import cv2
import pytesseract
from PIL import Image

import json
import requests

imgread = cv2.imread('/Users/jalma/OneDrive/Documents/testphotos/tmpx5z2c17c.png')
pytesseract.pytesseract.tesseract_cmd = (r'C:\Users\jalma\AppData\Local\Programs\Python\Python39\Scripts')

req = requests.get('https://ptsv2.com/t/synopsysmeter_1')

w = 31
h = 189

ptskeys = []
lastnine = []
basetobinary = []
tdlist = []
msgid = ""
final = ""

z = req.text
soup = BeautifulSoup(z, 'html.parser')

for tbody in soup.find_all('tbody'):
    for tr in tbody.find_all('tr'):
        tds = tr.find_all('td')
        ptskeys.append(tds[0].text)
lastnine = ptskeys[-8:-1]
print(lastnine)
msgid = lastnine[:1]
msgid2 = msgid[0]
print(msgid2)
print("\n")


for uk in lastnine:
    url = 'https://ptsv2.com/t/synopsysmeter_1/d/' + uk
    req2 = requests.get(url)
    y = req2.text
    reqsoup = BeautifulSoup(y, 'html.parser')
    tdlistvalue = reqsoup.find_all('td')
    del tdlistvalue[0:31]
    del tdlistvalue[1:]
    tdlistvalue = (tdlistvalue[0].text).strip()
    tdlist.append(tdlistvalue)
#print(tdlist)



arr_2d = np.zeros((w, h), dtype=np.uint8)

arr2dinteger = np.zeros((w, h), dtype=np.uint8)

for convert in tdlist:
    lol = str(convert).encode("ascii")
    decoded = base64.decodebytes(lol)
    line = ("".join(["{:08b}".format(x) for x in decoded]))
    #print(line)
    final = final + line

final = list(final)
del final[5859:]
print(final)
arr = np.array(list(final))

arr_2d = np.reshape(arr, (w, h))
print(arr_2d)

for x in range(w):
    for y in range(h):
        if arr_2d[x,y] == '1':
            arr2dinteger[x,y] = 255
        else:
            arr2dinteger[x,y] = 0


img = Image.fromarray(arr2dinteger, 'L')
img_flip = ImageOps.flip(img)
img_flip.save('C:\\Users\\jalma\\OneDrive\\Desktop\\testmeterread.jpg')
#img_flip.show()





pytesseract.pytesseract.tesseract_cmd ='C:\\Program Files\\Tesseract-OCR\\tesseract.exe'
img = cv2.imread('C:\\Users\\jalma\\OneDrive\\Desktop\\testmeterread.jpg', cv2.COLOR_BGR2GRAY)
#img = cv2.imread('C:\\Users\\jalma\\OneDrive\\Desktop\\testmeterread.jpg', cv2.COLOR_BGR2GRAY)
rawdata = pytesseract.image_to_string(img)
rawdata2 = (rawdata.replace('O', '0'))
rawdata3 = (rawdata2.replace('o', '0'))
rawdata4 = rawdata3[0:7]


def has_numbers(inputString):
    return any(char.isdigit() for char in inputString)


if has_numbers(rawdata4):
    rawdata_final = int(rawdata4)
else:
    rawdata_final = int("-1")

#cv2.imshow('Result',img)
cv2.waitKey(0)
gc = gspread.service_account(filename='C:\\Users\\jalma\\OneDrive\\Desktop\\credentials.json')
sh = gc.open_by_key('1X-qNdgl1-wQlmvmtXIO7wAY7Mc7Dz1bic097YXHUeCY')
worksheet = sh.sheet1

now = datetime.datetime.now()
current_time = now.strftime("%H:%M:%S")
print(current_time)

current_date = datetime.date.today()
print(current_date)

realtime = str(current_time)
realdate = str(current_date)


#rawdata_time = [realdate, realtime, rawdata_final]
#rawdata_time = [msgid2, realdate, realtime, rawdata_final]
#worksheet.append_row(rawdata_time)
#worksheet.insert_row(rawdata_time, 3)
#print(rawdata_final)

ac_id = "ya29.A0ARrdaM9w55xBbj4sA4UBy7x5Q6jjO4R_1PAJZzti9NYcGpRb3ZFU3R68XGt5r4azF3xKJVvY6TrjZrMK3mxrM9K2Sq5Z0XfGl1DE1aRbeyEGJ-8lAj3EPkePvgvRK42Z2xYGdPyuBKvdmey3faGVvY4i1EfY"

headers = {"Authorization": "Bearer " + ac_id} #put ur access token after the word 'Bearer '
para = {
    #"name": "testmeterread.jpg", #file name to be uploaded
    "name": msgid2,  # file name to be uploaded
    "parents": ["12VZDlVuNMdn1tw03dnsEkGm4kQBgiJ2H"] # make a folder on drive in which you want to upload files; then open that folder; the last thing in present url will be folder id
}
files = {
    'data': ('metadata', json.dumps(para), 'application/json; charset=UTF-8'),
    'file': ('image/png',open("C:\\Users\\jalma\\OneDrive\\Desktop\\testmeterread.jpg", "rb")) # replace 'application/zip' by 'image/png' for png images; similarly 'image/jpeg' (also replace your file name)
}
r = requests.post(
    "https://www.googleapis.com/upload/drive/v3/files?uploadType=multipart",
    headers=headers,
    files=files
)

read_r = str(r.text)
urllist = read_r.split('"')
urlid = urllist[7:8]
urlid2 = urlid[0]
fin_url = ("https://drive.google.com/file/d/" + urlid2)
print(fin_url)

#rawdata_time = [realdate, realtime, rawdata_final]
rawdata_time = [msgid2, realdate, realtime, rawdata_final, fin_url]
#worksheet.append_row(rawdata_time)
worksheet.insert_row(rawdata_time, 3)
print(rawdata_final)

