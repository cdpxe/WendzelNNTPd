import unittest
import pytest
import base64
from appium.options.android import UiAutomator2Options
from appium import webdriver
# Options are available in Python client since v2.6.0
from appium.options.windows import WindowsOptions
from selenium.webdriver.support.ui import WebDriverWait
from selenium.webdriver.common.by import By
from pprint import pprint
import sys
import requests
import time
import pyautogui
from selenium.common.exceptions import ElementNotVisibleException, ElementNotSelectableException, NoSuchElementException
from selenium.webdriver.support.wait import WebDriverWait

appium_server_url = 'http://127.0.0.1:4723'

capabilities = dict(
    platformName='Windows',
    automationName='Windows',
    app='C:\\Program Files\\Mozilla Thunderbird\\thunderbird.exe'
)

class TestAppium(unittest.TestCase):

    def setUp(self) -> None:
        self.driver = webdriver.Remote(appium_server_url, options=UiAutomator2Options().load_capabilities(capabilities))
        self.driver.implicitly_wait(30)
        self.driver.start_recording_screen()

    def save_video(self, video, filename):
        log_folder = 'D:\\a\\WendzelNNTPd\\WendzelNNTPd\\user-acceptance-test\\videos'
        video_path = f'{log_folder}/{filename}.mp4'
        with open(video_path, 'wb') as output_file:
            output_file.write(base64.b64decode(video))

    def tearDown(self) -> None:
        self.save_video(self.driver.stop_recording_screen(), 'test')
        if self.driver:
            self.driver.quit()

    def test_with_nntp(self) -> None:
        wait = WebDriverWait(self.driver, 30)

        wait.until(lambda x: x.find_element(By.XPATH, '//Button[@AutomationId="button-appmenu"]')).click()

        wait.until(lambda x: x.find_element('name', 'New Account')).click()

        wait.until(lambda x: x.find_element('name', 'Newsgroup')).click()

        wait.until(lambda x: x.find_element(By.XPATH, '//Edit[@Name="Your Name:"]')).send_keys('Testautomatisierung')

        wait.until(lambda x: x.find_element(By.XPATH, '//Edit[@Name="Email Address:"]')).send_keys('testautomatisierung@test.de')
        wait.until(lambda x: x.find_element(By.XPATH, '//Window[@AutomationId="AccountWizard"]/Button[contains(@Name, "Next")]')).click()

        wait.until(lambda x: x.find_element(By.XPATH, '//Edit[@Name="Newsgroup Server:"]')).send_keys('nntp.svenliebert.de')

        wait.until(lambda x: x.find_element(By.XPATH, '//Window[@AutomationId="AccountWizard"]/Button[contains(@Name, "Next")]')).click()
        
        wait.until(lambda x: x.find_element(By.XPATH, '//Window[@AutomationId="AccountWizard"]/Button[contains(@Name, "Next")]')).click()

        wait.until(lambda x: x.find_element(By.XPATH, '//Window[@AutomationId="AccountWizard"]/Button[contains(@Name, "Finish")]')).click()

        wait.until(lambda x: x.find_element(By.XPATH, '//Button[@AutomationId="mailButton"]')).click()

        wait.until(lambda x: x.find_element(By.XPATH, '//*[@Name="nntp.svenliebert.de"]')).click()

        # set TLS encryption begin
        wait.until(lambda x: x.find_element(By.XPATH, '//Button[@Name="Account Settings"]')).click()

        wait.until(lambda x: x.find_element(By.XPATH, '//TreeItem[@Name="Server Settings"]')).click()
        
        wait.until(lambda x: x.find_element(By.XPATH, '//ComboBox[@AutomationId="server.socketType"]')).click()

        time.sleep(5)
        pyautogui.press('down')
        pyautogui.press('enter')

        wait.until(lambda x: x.find_element(By.XPATH, '//Button[@AutomationId="mailButton"]')).click()

        wait.until(lambda x: x.find_element(By.XPATH, '//*[@Name="nntp.svenliebert.de"]')).click()
        # set TLS encryption end

        wait.until(lambda x: x.find_element(By.XPATH, '//Button[@AutomationId="nntpSubscriptionButton"]')).click()

        wait.until(lambda x: x.find_element(By.XPATH, '//TreeItem[@Name="alt.wendzelnntpd.test false"]/DataItem[@Name="false"]')).click()

        wait.until(lambda x: x.find_element(By.XPATH, '//Button[@Name="Subscribe"]')).click()
        wait.until(lambda x: x.find_element(By.XPATH, '//Button[@Name="OK"]')).click()


        wait.until(lambda x: x.find_element(By.XPATH, '//TreeItem[@Name="alt.wendzelnntpd.test"]')).click()
        wait.until(lambda x: x.find_element(By.XPATH, '//Button[@AutomationId="folderPaneGetMessages"]')).click()

        wait.until(lambda x: x.find_element(By.XPATH, '//Button[@AutomationId="folderPaneWriteMessage"]')).click()

        subject = 'Testautomatisierung - ' + str(time.time())

        time.sleep(5)

        pyautogui.write(subject)
        pyautogui.press('tab')
        pyautogui.write("Das ist ein Test")
        pyautogui.hotkey('ctrlleft', 'enter')
        pyautogui.press('enter')


        wait.until(lambda x: x.find_element(By.XPATH, '//Button[@AutomationId="folderPaneGetMessages"]')).click()

        element = wait.until(lambda x: x.find_element(By.XPATH, '//Group[@AutomationId="threadTree"]/Table/Tree[@Name="a.w.test"]/TreeItem[starts-with(@Name,"Testautomatisierung")]'))

        assert subject in element.text
if __name__ == '__main__':
    unittest.main()