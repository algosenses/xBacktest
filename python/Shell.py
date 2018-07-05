from xBacktest import *

if __name__ == '__main__':
    framework = xBacktest.Framework.instance()

    if framework.loadScenario('smacross.sce'):
        framework.run()

    framework.release()

