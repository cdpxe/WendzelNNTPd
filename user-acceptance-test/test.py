# Python3 + PyTest
import pytest

from appium import webdriver
# Options are available in Python client since v2.6.0
from appium.options.windows import WindowsOptions

def generate_options():
    classic_options = WindowsOptions()
    classic_options.app = 'C:\\Windows\\system32\\notepad.exe'
    # Make sure arguments are quoted/escaped properly if necessary:
    # https://ss64.com/nt/syntax-esc.html

    return [classic_options]


@pytest.fixture(params=generate_options())
def driver(request):
    # The default URL is http://127.0.0.1:4723/wd/hub in Appium 1
    drv = webdriver.Remote('http://127.0.0.1:4723', options=request.param)
    yield drv
    drv.quit()

def test_app_source_could_be_retrieved(driver):
    element = driver.find_element('name', 'File')
    element.click()
    #assert driver.find_element('name', 'Konten-Einstellungen')
