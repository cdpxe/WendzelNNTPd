import unittest
import pytest
import base64
from appium.options.android import UiAutomator2Options
from appium import webdriver
# Options are available in Python client since v2.6.0
from appium.options.windows import WindowsOptions

appium_server_url = 'http://127.0.0.1:4723'

capabilities = dict(
    platformName='Windows',
    automationName='Windows',
    app='C:\\Program Files\\Mozilla Thunderbird\\thunderbird.exe'
)

class TestAppium(unittest.TestCase):

    def setUp(self) -> None:
        self.driver = webdriver.Remote(appium_server_url, options=UiAutomator2Options().load_capabilities(capabilities))
        self.driver.start_recording_screen()

    def save_video(self, video, filename):
        log_folder = 'D:\\a\\SvenLie\\WendzelNNTPd\\user-acceptance-test\\videos'
        video_path = f'{log_folder}\\{filename}.mp4'
        with open(video_path, 'wb') as output_file:
            output_file.write(base64.b64decode(video))

    def tearDown(self) -> None:
        self.save_video(self.driver.stop_recording_screen(), 'test')
        if self.driver:
            self.driver.quit()

    def test_find_battery(self) -> None:
        self.driver.find_element('name', 'AppMenu').click()
        assert self.driver.find_element('name', 'New Account')
        self.driver.find_element('name', 'New Account').click()
        assert self.driver.find_element('name', 'Newsgroup')
        self.driver.find_element('name', 'Newsgroup').click()

if __name__ == '__main__':
    unittest.main()